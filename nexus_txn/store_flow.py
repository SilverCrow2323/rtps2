"""Explicit Store screen workflow state machine; no device I/O or installer execution."""
from dataclasses import dataclass, field
NEXT = {
 "SELECT":{"DEPENDENCIES","CANCELLED"},"DEPENDENCIES":{"SPACE","CANCELLED"},
 "SPACE":{"CONFIRM","CANCELLED"},"CONFIRM":{"DOWNLOAD","CANCELLED"},
 "DOWNLOAD":{"VERIFY","FAILED","CANCELLED"},"VERIFY":{"READY_TO_INSTALL","FAILED"},
 "READY_TO_INSTALL":{"DONE","CANCELLED"},"DONE":set(),"CANCELLED":set(),"FAILED":set()
}
class StoreFlowError(RuntimeError): pass
@dataclass
class StoreFlow:
 selected_ids:list[str]=field(default_factory=list)
 stage:str="SELECT"
 space_check:str="unknown"
 progress:int=0
 verified:bool=False
 events:list[str]=field(default_factory=list)
 def advance(self,stage):
  if stage not in NEXT.get(self.stage,set()): raise StoreFlowError(f"invalid transition: {self.stage} -> {stage}")
  if stage=="DEPENDENCIES" and not self.selected_ids: raise StoreFlowError("select at least one package first")
  if stage=="CONFIRM" and self.space_check!="pass": raise StoreFlowError("space check must pass before confirmation")
  if stage=="VERIFY" and self.progress!=100: raise StoreFlowError("download must reach 100% before verification")
  if stage=="READY_TO_INSTALL" and not self.verified: raise StoreFlowError("package must be verified before install-ready state")
  self.stage=stage; self.events.append(stage); return self.snapshot()
 def set_space_check(self,result):
  if self.stage!="SPACE" or result not in {"pass","insufficient","unknown"}: raise StoreFlowError("space result can only be set during SPACE")
  self.space_check=result
 def set_progress(self,percent):
  if self.stage!="DOWNLOAD" or not isinstance(percent,int) or isinstance(percent,bool) or not 0<=percent<=100: raise StoreFlowError("progress must be 0..100 during DOWNLOAD")
  if percent<self.progress: raise StoreFlowError("download progress cannot move backwards")
  self.progress=percent
 def mark_verified(self,valid):
  if self.stage!="VERIFY": raise StoreFlowError("verification result outside VERIFY stage")
  if valid is not True: self.advance("FAILED"); return
  self.verified=True
 def snapshot(self):
  return {"stage":self.stage,"selected_ids":list(self.selected_ids),"space_check":self.space_check,
          "progress":self.progress,"verified":self.verified,"device_write_performed":False,
          "installer_execution_enabled":False,"events":list(self.events)}
