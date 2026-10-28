// Cloudflare Worker: turns a student's "Send to teacher" report into a GitHub Issue.
// Deploy once (free): workers.cloudflare.com -> Create Worker -> paste this -> Deploy.
// Settings -> Variables:  REPO = Heshamoov/Studica   GITHUB_TOKEN = (secret) fine-grained token, Issues: read+write on that repo.
// Then put the worker URL in config.json -> "report_url".
export default {
  async fetch(req, env) {
    const cors = { "Access-Control-Allow-Origin": "*", "Access-Control-Allow-Methods": "POST,OPTIONS", "Access-Control-Allow-Headers": "content-type" };
    if (req.method === "OPTIONS") return new Response(null, { headers: cors });
    if (req.method !== "POST") return new Response("studica report relay", { headers: cors });
    let r; try { r = await req.json(); } catch (e) { return new Response("bad json", { status: 400, headers: cors }); }
    const team = String(r.team || "unknown").replace(/[^\w-]/g, "").slice(0, 30) || "unknown";
    const note = String(r.note || "").trim() || "(no description)";
    const cut = (s, n) => { s = String(s || ""); return s.length > n ? s.slice(-n) : s; };
    const title = `[${team}] ${note.split("\n")[0].slice(0, 80)}`;
    const body = [
      `**Team:** ${team}  ·  **Dashboard:** ${r.version || "?"}  ·  **Robot program:** ${r.robotVersion || "?"}  ·  **Time:** ${r.time || ""}`,
      "", "### What the student wrote", note,
      "", "### Robot state", "```json", cut(JSON.stringify(r.state, null, 1), 4000), "```",
      "", "### Helper status", "```json", cut(JSON.stringify(r.manager, null, 1), 1500), "```",
      "", "### Settings", "```json", cut(JSON.stringify(r.settings), 6000), "```",
      "", "### Robot log (last lines)", "```", cut(r.log, 12000), "```",
      "", "### Compile log (last lines)", "```", cut(r.buildLog, 6000), "```",
      "", "### Diagnostics", "```json", cut(JSON.stringify(r.diag, null, 1), 12000), "```",
    ].join("\n");
    const gh = await fetch(`https://api.github.com/repos/${env.REPO}/issues`, {
      method: "POST",
      headers: { Authorization: `Bearer ${env.GITHUB_TOKEN}`, Accept: "application/vnd.github+json", "Content-Type": "application/json", "User-Agent": "studica-report-relay" },
      body: JSON.stringify({ title, body, labels: ["report", team] }),
    });
    const out = gh.ok ? { ok: true, url: (await gh.json()).html_url } : { ok: false, status: gh.status };
    return new Response(JSON.stringify(out), { status: gh.ok ? 200 : 502, headers: { ...cors, "content-type": "application/json" } });
  },
};
