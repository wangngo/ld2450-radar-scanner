// Trang radar phục vụ từ ESP32 — điện thoại poll /data ~10 lần/giây.
#pragma once
static const char WEB_PAGE[] = R"HTML(<!doctype html>
<html lang="vi"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>LD2450 Radar</title>
<style>
:root{--bg:#04110a;--fg:#7dffb0;--dim:#1f6b42;--warn:#ff4d4d}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.35 ui-monospace,Menlo,monospace}
header{display:flex;justify-content:space-between;align-items:center;padding:8px 12px;border-bottom:1px solid var(--dim)}
#st{font-size:12px;color:var(--dim)}#st.ok{color:var(--fg)}#st.bad{color:var(--warn)}
canvas{display:block;width:100%;touch-action:none}
.row{display:flex;flex-wrap:wrap;gap:8px;padding:8px 12px;align-items:center}
button,select{background:#0a2416;color:var(--fg);border:1px solid var(--dim);border-radius:6px;padding:8px 10px;font:inherit}
button.on{background:var(--fg);color:var(--bg)}
input[type=range]{flex:1;min-width:120px;accent-color:var(--fg)}
table{width:calc(100% - 24px);margin:0 12px 12px;border-collapse:collapse}
td,th{border-bottom:1px solid var(--dim);padding:5px 4px;text-align:right}th{color:var(--dim);font-weight:normal}
td:first-child,th:first-child{text-align:left}
@media(max-width:430px){table{font-size:12px}td,th{padding:5px 2px}}
html,body{overflow-x:hidden}
body.alert{animation:fl .4s steps(2) infinite}@keyframes fl{50%{background:#3a0808}}
</style></head><body>
<header><b>LD2450 RADAR</b><span id="st">đang kết nối…</span></header>
<canvas id="c"></canvas>
<div class="row">
 <label>Tầm <select id="rng"><option>3</option><option selected>4</option><option>6</option></select> m</label>
 <button id="snd">🔇 Âm thanh</button>
 <button id="rec">● Ghi log</button>
 <button id="dl">⬇ CSV (0)</button>
</div>
<div class="row"><span>Vùng cảnh báo</span><input id="zone" type="range" min="0" max="6" step="0.25" value="1.5"><span id="zv">1.5 m</span></div>
<table><thead><tr><th>#</th><th>Cách</th><th>Góc</th><th>X</th><th>Y</th><th>V</th></tr></thead><tbody id="tb"></tbody></table>
<script>
const C=document.getElementById('c'),X=C.getContext('2d'),COL=['#7dffb0','#ffd84d','#5cc8ff'];
let R=4,zone=1.5,trails=[[],[],[]],targets=[],sound=false,ac=null,nextBeep=0,rec=false,log=[],lastOk=0,fps=0,cnt=0,t0=performance.now();
const $=id=>document.getElementById(id);
function size(){const w=C.clientWidth,d=devicePixelRatio||1;C.width=w*d;C.height=w*0.62*d;C.style.height=w*0.62+'px';X.setTransform(d,0,0,d,0,0)}
addEventListener('resize',size);size();
$('rng').onchange=e=>{R=+e.target.value;$('zone').max=R;if(zone>R){zone=R;$('zone').value=R;$('zv').textContent=R+' m'}};
$('zone').oninput=e=>{zone=+e.target.value;$('zv').textContent=zone+' m'};
$('snd').onclick=()=>{sound=!sound;if(sound&&!ac)ac=new (window.AudioContext||window.webkitAudioContext)();if(ac)ac.resume();$('snd').classList.toggle('on',sound);$('snd').textContent=(sound?'🔊':'🔇')+' Âm thanh'};
$('rec').onclick=()=>{rec=!rec;$('rec').classList.toggle('on',rec);$('rec').textContent=rec?'■ Dừng ghi':'● Ghi log'};
$('dl').onclick=()=>{if(!log.length)return;const b=new Blob(['ts_iso,id,x_mm,y_mm,speed_cms,dist_mm,angle_deg\n'+log.join('\n')],{type:'text/csv'});const a=document.createElement('a');a.href=URL.createObjectURL(b);a.download='ld2450_'+new Date().toISOString().replace(/[:.]/g,'-')+'.csv';a.click()};
function geo(){const w=C.clientWidth,h=w*0.62,cx=w/2,cy=h-6,rad=Math.min(h-12,(w/2-6)/Math.sin(Math.PI/3));return{w,h,cx,cy,rad}}
function toPx(g,x,y){return[g.cx+x/1000/R*g.rad,g.cy-y/1000/R*g.rad]}
function draw(){const g=geo(),a0=-Math.PI/2-Math.PI/3,a1=-Math.PI/2+Math.PI/3;X.clearRect(0,0,g.w,g.h);
 X.strokeStyle='#1f6b42';X.fillStyle='#1f6b42';X.lineWidth=1;X.font='11px monospace';
 const step=R<=3?0.5:1;for(let r=step;r<=R+1e-6;r+=step){const p=r/R*g.rad;X.beginPath();X.arc(g.cx,g.cy,p,a0,a1);X.stroke();X.fillText(r+'m',g.cx+3,g.cy-p+11)}
 for(const a of[-60,-30,0,30,60]){const t=(a-90)*Math.PI/180;X.beginPath();X.moveTo(g.cx,g.cy);X.lineTo(g.cx+Math.cos(t)*g.rad,g.cy+Math.sin(t)*g.rad);X.stroke()}
 if(zone>0){X.strokeStyle='#ff4d4d';X.setLineDash([4,4]);X.beginPath();X.arc(g.cx,g.cy,Math.min(zone,R)/R*g.rad,a0,a1);X.stroke();X.setLineDash([])}
 trails.forEach((tr,i)=>{tr.forEach((p,k)=>{const[px,py]=toPx(g,p[0],p[1]);X.globalAlpha=(k+1)/tr.length*0.5;X.fillStyle=COL[i];X.beginPath();X.arc(px,py,3,0,7);X.fill()});X.globalAlpha=1});
 targets.forEach(t=>{const[px,py]=toPx(g,t.x,t.y);X.fillStyle=COL[t.id-1];X.shadowColor=COL[t.id-1];X.shadowBlur=12;X.beginPath();X.arc(px,py,7,0,7);X.fill();X.shadowBlur=0;X.fillStyle='#04110a';X.fillText(t.id,px-3,py+4)});
 requestAnimationFrame(draw)}
requestAnimationFrame(draw);
function beep(f){const o=ac.createOscillator(),v=ac.createGain();o.frequency.value=f;o.connect(v);v.connect(ac.destination);const n=ac.currentTime;v.gain.setValueAtTime(0.25,n);v.gain.exponentialRampToValueAtTime(0.001,n+0.09);o.start(n);o.stop(n+0.1)}
function audio(){if(sound&&ac){const near=targets.filter(t=>t.d/1000<=R).sort((a,b)=>a.d-b.d)[0];const now=performance.now();if(near&&now>=nextBeep){const k=Math.min(1,near.d/1000/R);beep(1300-800*k);nextBeep=now+90+1300*k}}setTimeout(audio,30)}
audio();
async function poll(){try{const r=await fetch('/data',{cache:'no-store'});const j=await r.json();lastOk=performance.now();
 targets=j.t.map(a=>({id:a[0],x:a[1],y:a[2],s:a[3],d:Math.round(Math.hypot(a[1],a[2])),ang:Math.atan2(a[1],a[2])*57.2958}));
 const seen=new Set(targets.map(t=>t.id));trails=trails.map((tr,i)=>seen.has(i+1)?tr:[]);
 targets.forEach(t=>{const tr=trails[t.id-1];tr.push([t.x,t.y]);if(tr.length>10)tr.shift()});
 if(rec){const ts=new Date().toISOString();targets.forEach(t=>log.push([ts,t.id,t.x,t.y,t.s,t.d,t.ang.toFixed(1)].join(',')));$('dl').textContent='⬇ CSV ('+log.length+')'}
 document.body.classList.toggle('alert',zone>0&&targets.some(t=>t.d/1000<=zone));
 $('tb').innerHTML=targets.map(t=>`<tr><td style="color:${COL[t.id-1]}">${t.id}</td><td>${(t.d/1000).toFixed(2)} m</td><td>${t.ang.toFixed(0)}°</td><td>${(t.x/1000).toFixed(2)}</td><td>${(t.y/1000).toFixed(2)}</td><td>${t.s} cm/s</td></tr>`).join('')||'<tr><td colspan="6" style="text-align:center;color:#1f6b42">không có mục tiêu</td></tr>';
 cnt++;const n=performance.now();if(n-t0>1000){fps=cnt*1000/(n-t0);cnt=0;t0=n}
 const st=$('st');if(j.stale){st.textContent='radar không gửi dữ liệu';st.className='bad'}else{st.textContent=fps.toFixed(0)+' Hz · '+j.n+' frame';st.className='ok'}
}catch(e){const st=$('st');st.textContent='mất kết nối ESP32';st.className='bad'}
 setTimeout(poll,100)}
poll();
</script></body></html>)HTML";
