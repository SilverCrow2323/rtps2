"""Small terminal-screen renderer for a future NEXUS Store TUI adapter."""
def render_progress(title, phase, percent, detail="", width=28):
    if not isinstance(percent, int) or isinstance(percent, bool) or not 0 <= percent <= 100:
        raise ValueError("percent must be an integer from 0 to 100")
    if width < 8:
        raise ValueError("width must be at least 8")
    filled = (width * percent) // 100
    bar = "#" * filled + "-" * (width - filled)
    lines = ["+" + "-" * (width + 2) + "+", f"| {title[:width]:<{width}} |",
             f"| {phase[:width]:<{width}} |", f"| [{bar}] {percent:3d}% |"]
    if detail:
        lines.append(f"| {detail[:width]:<{width}} |")
    lines.append("+" + "-" * (width + 2) + "+")
    return "\n".join(lines)

def render_plan(plan):
    lines = ["NEXUS STORE / PREVIEW", "=" * 24]
    for item in plan["actions"]:
        lines.append(f"{item['action'].upper():7} {item['id']}  {item['version']}")
    lines += ["-" * 24, f"Required: {plan['required_bytes']} bytes",
              f"Free: {plan['free_bytes'] if plan['free_bytes'] is not None else 'unknown'}",
              f"Space: {plan['space_check']}", "NO DEVICE WRITE / NO DOWNLOAD"]
    return "\n".join(lines)
