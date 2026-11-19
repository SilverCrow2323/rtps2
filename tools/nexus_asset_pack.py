#!/usr/bin/env python3
"""Build and verify deterministic NEXUS GS-ready PSMCT32 icon atlases."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
import struct
import sys
import tempfile
import zlib
from pathlib import Path

PNG_SIGNATURE=b"\x89PNG\r\n\x1a\n"
HEADER=struct.Struct("<4sHHHHHHI32s12s")
HEADER_SIZE=HEADER.size
MAGIC=b"NXA1"
VERSION=1
PSMCT32=0
ATLAS_WIDTH, ATLAS_HEIGHT, ICON_SIZE = 256, 128, 64
ICON_NAMES=("games","ps1","homebrew","store","files","system")
MAX_DIMENSION=1024
MAX_COMPRESSED=4*1024*1024

class AssetError(ValueError):
    pass

def paeth(a,b,c):
    p=a+b-c
    pa,pb,pc=abs(p-a),abs(p-b),abs(p-c)
    return a if pa<=pb and pa<=pc else (b if pb<=pc else c)

def decode_png_rgba(data):
    """Decode non-interlaced 8-bit RGB/RGBA PNG with CRC and size checks."""
    if not data.startswith(PNG_SIGNATURE):
        raise AssetError("not a PNG file")
    pos=len(PNG_SIGNATURE); width=height=color=None; idat=bytearray()
    ihdr=iend=False
    while pos<len(data):
        if pos+12>len(data): raise AssetError("truncated PNG chunk header")
        length=struct.unpack_from(">I",data,pos)[0]
        kind=data[pos+4:pos+8]; end=pos+8+length
        if end+4>len(data): raise AssetError("truncated PNG chunk")
        chunk=data[pos+8:end]
        crc=struct.unpack_from(">I",data,end)[0]
        if zlib.crc32(kind+chunk)&0xffffffff != crc:
            raise AssetError("PNG CRC mismatch")
        if kind==b"IHDR":
            if ihdr or pos!=8 or length!=13: raise AssetError("invalid PNG IHDR")
            width,height,depth,color,compression,flt,interlace=struct.unpack(">IIBBBBB",chunk)
            if not (1<=width<=MAX_DIMENSION and 1<=height<=MAX_DIMENSION):
                raise AssetError("PNG dimensions outside safety limit")
            if depth!=8 or color not in (2,6): raise AssetError("only 8-bit RGB/RGBA PNGs are supported")
            if (compression,flt,interlace)!=(0,0,0): raise AssetError("unsupported PNG compression, filter or interlace")
            ihdr=True
        elif kind==b"IDAT":
            if not ihdr or iend: raise AssetError("PNG IDAT outside image data")
            idat.extend(chunk)
            if len(idat)>MAX_COMPRESSED: raise AssetError("compressed PNG exceeds safety limit")
        elif kind==b"IEND":
            if length or not ihdr: raise AssetError("invalid PNG IEND")
            iend=True; pos=end+4; break
        elif kind[0]&0x20==0 and kind!=b"PLTE":
            raise AssetError("unsupported critical PNG chunk: "+repr(kind))
        pos=end+4
    if not ihdr or not iend or pos!=len(data) or not idat:
        raise AssetError("incomplete PNG or unexpected trailing data")
    channels=4 if color==6 else 3
    stride=width*channels
    expected=height*(stride+1)
    try:
        decoder=zlib.decompressobj()
        raw=decoder.decompress(bytes(idat),expected+1)
        if len(raw)>expected or decoder.unconsumed_tail:
            raise AssetError("PNG decompressed data exceeds IHDR dimensions")
        raw+=decoder.flush(expected+1-len(raw))
    except zlib.error as exc:
        raise AssetError("invalid PNG deflate stream") from exc
    if len(raw)!=expected or not decoder.eof or decoder.unused_data:
        raise AssetError("PNG decoded size mismatch or trailing deflate data")
    decoded=bytearray(height*stride); cursor=0
    for y in range(height):
        ft=raw[cursor]; cursor+=1
        scan=raw[cursor:cursor+stride]; cursor+=stride
        row=y*stride; prev=row-stride
        for x,v in enumerate(scan):
            left=decoded[row+x-channels] if x>=channels else 0
            up=decoded[prev+x] if y else 0
            ul=decoded[prev+x-channels] if y and x>=channels else 0
            if ft==1: v+=left
            elif ft==2: v+=up
            elif ft==3: v+=(left+up)//2
            elif ft==4: v+=paeth(left,up,ul)
            elif ft!=0: raise AssetError("unsupported PNG filter "+str(ft))
            decoded[row+x]=v&255
    if channels==4: return width,height,bytes(decoded)
    rgba=bytearray(width*height*4); dst=0
    for src in range(0,len(decoded),3):
        rgba[dst:dst+3]=decoded[src:src+3]; rgba[dst+3]=255; dst+=4
    return width,height,bytes(rgba)

def png_chunk(kind,payload):
    return struct.pack(">I",len(payload))+kind+payload+struct.pack(">I",zlib.crc32(kind+payload)&0xffffffff)

def encode_png_rgba(width,height,pixels):
    if len(pixels)!=width*height*4: raise AssetError("PNG pixel size mismatch")
    rows=b"".join(b"\0"+pixels[y*width*4:(y+1)*width*4] for y in range(height))
    ihdr=struct.pack(">IIBBBBB",width,height,8,6,0,0,0)
    return PNG_SIGNATURE+png_chunk(b"IHDR",ihdr)+png_chunk(b"IDAT",zlib.compress(rows,9))+png_chunk(b"IEND",b"")

def build_icon_atlas(icon_dir):
    atlas=bytearray(ATLAS_WIDTH*ATLAS_HEIGHT*4); manifest={}
    for i,name in enumerate(ICON_NAMES):
        path=Path(icon_dir)/(name+".png")
        try: source=path.read_bytes()
        except OSError as exc: raise AssetError("missing icon source "+str(path)) from exc
        w,h,rgba=decode_png_rgba(source)
        if (w,h)!=(ICON_SIZE,ICON_SIZE):
            raise AssetError(f"{name}.png must be exactly 64x64, got {w}x{h}")
        x=(i%4)*ICON_SIZE; y=(i//4)*ICON_SIZE
        for row in range(ICON_SIZE):
            src=row*ICON_SIZE*4; dst=((y+row)*ATLAS_WIDTH+x)*4
            atlas[dst:dst+ICON_SIZE*4]=rgba[src:src+ICON_SIZE*4]
        manifest[name]={"source":path.name,"source_sha256":hashlib.sha256(source).hexdigest(),
                        "x":x,"y":y,"width":ICON_SIZE,"height":ICON_SIZE}
    # GS PSMCT32 uses an 8-bit alpha value whose opaque endpoint is 0x80.
    for alpha in range(3,len(atlas),4):
        atlas[alpha]=(atlas[alpha]*128+127)//255
    return bytes(atlas),manifest

def make_pack(width,height,pixels):
    if not (1<=width<=1024 and 1<=height<=1024) or width&(width-1) or height&(height-1):
        raise AssetError("GS texture dimensions must be powers of two in range 1..1024")
    size=width*height*4
    if len(pixels)!=size or size>MAX_COMPRESSED: raise AssetError("invalid or oversized RGBA32 payload")
    digest=hashlib.sha256(pixels).digest()
    return HEADER.pack(MAGIC,VERSION,HEADER.size,width,height,PSMCT32,0,size,digest,b"\0"*12)+pixels

def verify_pack(data):
    if len(data)<HEADER.size: raise AssetError("truncated NXA header")
    magic,version,hs,w,h,psm,flags,n,digest,reserved=HEADER.unpack_from(data)
    if (magic,version,hs)!=(MAGIC,VERSION,HEADER.size): raise AssetError("unsupported NXA magic/version/header")
    if psm!=PSMCT32 or flags or reserved!=b"\0"*12: raise AssetError("unsupported NXA format/flags/reserved fields")
    if not (1<=w<=1024 and 1<=h<=1024) or w&(w-1) or h&(h-1): raise AssetError("invalid NXA dimensions")
    if n!=w*h*4 or n>MAX_COMPRESSED or len(data)!=hs+n: raise AssetError("NXA payload length mismatch")
    actual=hashlib.sha256(data[hs:]).digest()
    if actual!=digest: raise AssetError("NXA SHA-256 mismatch")
    return {"format":"NXA1","version":version,"width":w,"height":h,"pixel_format":"PSMCT32_RGBA",
            "payload_bytes":n,"sha256":actual.hex()}

def atomic_write(path,data):
    path=Path(path); path.parent.mkdir(parents=True,exist_ok=True)
    fd,tmp=tempfile.mkstemp(prefix="."+path.name+".",dir=str(path.parent))
    try:
        with os.fdopen(fd,"wb") as f: f.write(data); f.flush(); os.fsync(f.fileno())
        os.replace(tmp,path)
    except Exception:
        try: os.unlink(tmp)
        except OSError: pass
        raise

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest="cmd",required=True)
    pack=sub.add_parser("pack",help="build the six-icon 256x128 GS atlas")
    pack.add_argument("--icons",type=Path,required=True)
    pack.add_argument("--output",type=Path,required=True)
    pack.add_argument("--preview-png",type=Path)
    pack.add_argument("--manifest",type=Path)
    check=sub.add_parser("verify",help="validate an NXA file and payload digest")
    check.add_argument("pack",type=Path)
    args=parser.parse_args(argv)
    try:
        if args.cmd=="verify":
            print(json.dumps(verify_pack(args.pack.read_bytes()),indent=2)); return 0
        payload,icons=build_icon_atlas(args.icons)
        pack_data=make_pack(ATLAS_WIDTH,ATLAS_HEIGHT,payload)
        summary=verify_pack(pack_data)
        atomic_write(args.output,pack_data)
        if args.preview_png:
            preview=bytearray(payload)
            for i in range(3,len(preview),4): preview[i]=min(255,(preview[i]*255+64)//128)
            atomic_write(args.preview_png,encode_png_rgba(ATLAS_WIDTH,ATLAS_HEIGHT,bytes(preview)))
        manifest={"format":"NXA1","version":VERSION,"pixel_format":"PSMCT32_RGBA",
                  "alpha":"PNG 0..255 scaled to GS 0..128",
                  "atlas":{"width":ATLAS_WIDTH,"height":ATLAS_HEIGHT,"bytes":len(payload)},
                  "sha256":summary["sha256"],"icons":icons,"unused_tiles":2,
                  "header_bytes":HEADER.size,"pack_bytes":len(pack_data),
                  "pack_sha256":hashlib.sha256(pack_data).hexdigest()}
        if args.manifest: atomic_write(args.manifest,(json.dumps(manifest,indent=2)+"\n").encode())
        print(json.dumps({**summary,"pack_bytes":len(pack_data),
                          "pack_sha256":hashlib.sha256(pack_data).hexdigest()},indent=2))
        return 0
    except (AssetError,OSError,struct.error) as exc:
        print("nexus_asset_pack: error: "+str(exc),file=sys.stderr); return 2

if __name__=="__main__":
    raise SystemExit(main())
