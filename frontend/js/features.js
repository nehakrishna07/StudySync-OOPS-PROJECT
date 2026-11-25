// EXTRA FEATURES: browser controls only. All scheduling and data changes
// are calculated/stored by the C++ server through its JSON API.
const niceHours = h => `${Number(h).toFixed(1).replace(/\.0$/, '')}h`;
const futureDate = days => isoDay(shiftDate(new Date(), days));

// The simulator never saves a task. It compares two real C++ schedules.
function simulatorPage() {
    return pageTop('What-if Simulator', 'Try an assignment without adding it to your task list.') + `
    <div class="card section"><h2>Hypothetical task</h2>
      <form id="simulate-form" class="compact-form"><div class="form-grid">
        <label>Task name<input name="title" value="Possible new project" required maxlength="200"></label>
        <label>Subject<input name="subject" value="General"></label>
        <label>Deadline<input name="deadline" type="date" min="${futureDate(0)}" value="${futureDate(5)}" required></label>
        <label>Estimated hours<input name="estimated" type="number" min="0.5" max="10000" step="0.5" value="10" required></label>
        <label>Category<select name="category">${['Assignment','Project','Lab Work','Exam','Revision'].map(x=>`<option>${x}</option>`).join('')}</select></label>
        <label>Priority<select name="priority"><option>Low</option><option selected>Medium</option><option>High</option></select></label>
        <label>Difficulty<select name="difficulty"><option>Easy</option><option selected>Medium</option><option>Hard</option></select></label>
      </div><button class="primary">Run simulation</button><p class="sub">Your saved tasks will not be changed.</p></form>
      <div id="sim-result" aria-live="polite"></div></div>`;
}
async function simulateSubmit(e) {
    e.preventDefault();
    const data = Object.fromEntries(new FormData(e.target).entries());
    data.estimated = Number(data.estimated);
    const box = $('#sim-result');
    box.textContent = 'Calculating...';
    try {
        const result = await api('simulate','POST',data);
        const before = result.before, after = result.after;
        const extra = after.unallocated - before.unallocated;
        const simTask = after.explanations.find(x => x.taskId === -1);
        box.innerHTML = `<div class="feature-results"><div class="grid3">
          ${stat('Before: unallocated',niceHours(before.unallocated),'Current plan')}
          ${stat('After: unallocated',niceHours(after.unallocated),'With the hypothetical task')}
          ${stat('Additional shortage',niceHours(Math.max(0,extra)),'Impact on all tasks')}
        </div><div class="notice ${after.unallocated?'notice-warn':''}"><strong>${after.unallocated?'Some hours do not fit':'The simulated plan fits'}</strong><p>${esc(simTask?.reason||'')}</p></div>
        <p class="sub">Simulation only — nothing was saved. The comparison respects each task’s deadline and daily capacity.</p></div>`;
    } catch(e) { box.textContent = e.message; }
}

// Simple focus timer: only writes to SQLite when the student chooses to log.
let focusSeconds = 25 * 60;
let focusRunning = false;
let focusStartedSeconds = 25 * 60;
let focusInterval = null;
let focusIsBreak = false;
function timerDisplay() {
    const clock = $('#focus-clock');
    if (!clock) return;
    clock.textContent = `${String(Math.floor(focusSeconds/60)).padStart(2,'0')}:${String(focusSeconds%60).padStart(2,'0')}`;
    $('#focus-start').textContent = focusRunning ? 'Pause' : 'Start';
}
function timerPage() {
    const choices = tasks.filter(t=>t.status!=='Completed').map(t=>`<option value="${t.id}">${esc(t.title)}</option>`).join('');
    return pageTop('Focus Timer', 'Study in focused sessions and log your actual time.') + `<div class="card section timer-panel">
      <label>Study task<select id="focus-task">${choices||'<option value="">Add a task first</option>'}</select></label>
      <label>Session length (minutes)<input id="focus-duration" type="number" min="1" max="180" value="25"></label>
      <div class="timer-clock" id="focus-clock">25:00</div><p class="sub">Use a 25-minute session and take a short break afterward.</p>
      <div class="form-actions"><button class="primary" id="focus-start" ${choices?'':'disabled'}>Start</button>
      <button class="secondary" id="focus-reset">Reset</button>
      <button class="secondary" id="focus-break">5m break</button>
      <button class="secondary" id="focus-log" ${choices?'':'disabled'}>Finish & log</button></div>
      <p id="focus-message" class="sub" aria-live="polite">Logging study minutes reduces the task's remaining estimated hours.</p>
    </div>`;
}
function focusStart() {
    if (focusRunning) { focusRunning=false; clearInterval(focusInterval); timerDisplay(); return; }
    if (focusSeconds<=0) { $('#focus-message').textContent=focusIsBreak?'Break finished. Reset to study.':'Session finished. Log your work or reset the timer.'; return; }
    focusRunning=true;
    focusInterval=setInterval(()=>{
        focusSeconds=Math.max(0,focusSeconds-1);
        if (focusSeconds===0) { clearInterval(focusInterval); focusRunning=false; $('#focus-message') && ($('#focus-message').textContent=focusIsBreak?'Break finished! Reset to study.':'Session complete! Use Finish & log to record your work.'); }
        timerDisplay();
    },1000);
    timerDisplay();
}
function focusReset() {
    clearInterval(focusInterval);focusRunning=false;focusIsBreak=false;
    const minutes = Math.max(1,Math.min(180,Number($('#focus-duration')?.value)||25));
    focusStartedSeconds=focusSeconds=Math.round(minutes*60);
    timerDisplay();
}
function focusBreak() {
    clearInterval(focusInterval);focusRunning=false;focusIsBreak=true;
    focusSeconds=focusStartedSeconds=5*60;
    $('#focus-message').textContent='Five-minute break. Break time is never logged as study time.';
    timerDisplay();
}
async function recordMinutes(taskId, minutes) {
    await api('time-logs','POST',{taskId:Number(taskId),minutes:Number(minutes)});
    await refresh();
}
async function focusLog() {
    const taskId=$('#focus-task')?.value;
    if (!taskId || focusIsBreak) { $('#focus-message').textContent='Start a study session before logging time.'; return; }
    const minutes=Math.ceil((focusStartedSeconds-focusSeconds)/60);
    if (minutes<1) { $('#focus-message').textContent='Run the timer for at least one minute before logging.'; return; }
    try {
        await recordMinutes(taskId,minutes);
        focusReset();
        $('#focus-message').textContent=`Saved ${minutes} study minutes. The task and schedule were updated.`;
    } catch(e) { $('#focus-message').textContent=e.message; }
}
function focusBind() {
    $('#focus-start').onclick=focusStart;
    $('#focus-reset').onclick=focusReset;
    $('#focus-break').onclick=focusBreak;
    $('#focus-log').onclick=focusLog;
    $('#focus-duration').onchange=focusReset;
    timerDisplay();
}

// Actual logged minutes come from the database, not local browser storage.
function trackingPage() {
    return pageTop('Time Tracking', 'Compare your estimates with real study time.') + `
      <div class="card section"><h2>Record study time manually</h2>
      <form id="manual-log" class="compact-form"><div class="form-grid">
      <label>Task<select name="taskId" required>${tasks.filter(t=>t.status!=='Completed').map(t=>`<option value="${t.id}">${esc(t.title)}</option>`).join('')}</select></label>
      <label>Actual minutes<input type="number" name="minutes" min="1" max="1440" step="1" value="30" required></label>
      </div><button class="primary">Log study time</button></form></div>
      <div id="tracking-data"><div class="card section">Loading study history...</div></div>`;
}
async function loadTracking() {
    const container=$('#tracking-data');
    if (!container) return;
    try {
        const {logs,totals: aggregates}=await api('time-logs');
        const totals = new Map(aggregates.map(x => [x.taskId,x.minutes]));
        container.innerHTML=`<div class="card section"><h2>Estimated vs actual time</h2>
          <p class="sub">Actual totals include every recorded session (the history shows the latest 100). Estimated time is your original estimate.</p>
          <div class="table-scroll"><table><thead><tr><th>Task</th><th>Estimated</th><th>Actually studied</th><th>Remaining</th></tr></thead><tbody>
          ${tasks.map(t=>`<tr><td>${esc(t.title)}</td><td>${niceHours(t.estimated)}</td><td>${niceHours((totals.get(t.id)||0)/60)}</td><td>${niceHours(t.remaining)}</td></tr>`).join('')}</tbody></table></div></div>
          <div class="card section"><h2>Recent study sessions</h2>${logs.map(l=>`<div class="session"><span>${esc(l.title)}<br><small>${esc(l.loggedAt)}</small></span><strong>${l.minutes} min</strong></div>`).join('')||'<p class="empty">No study time logged yet.</p>'}</div>`;
    } catch(e) { container.textContent=e.message; }
}
async function manualLog(e) {
    e.preventDefault();
    try { const f=new FormData(e.target);await recordMinutes(f.get('taskId'),Number(f.get('minutes')));show('tracking'); }
    catch(error) { alert(error.message); }
}

function explanationPanel() {
    const explanations=analysis.explanations||[];
    return `<div class="card section"><h2>Why this schedule?</h2><p class="sub">StudySync allocates deadlines first, then prioritises tasks with the same deadline. These explanations are calculated in C++.</p>
      ${explanations.map(x=>`<details class="explain"><summary>${esc(x.title)} <span class="muted">${niceHours(x.allocated)} scheduled · ${niceHours(x.unallocated)} unallocated</span></summary><p>${esc(x.reason)}</p></details>`).join('')||'<p class="empty">Add a task to see its scheduling explanation.</p>'}</div>`;
}

function detailedDay(dateKey) {
    const slots=(analysis.sessions||[]).filter(s=>s.date===dateKey);
    const max=availability[weekdayForDate(dateKey)];
    const total=slots.reduce((a,s)=>a+s.hours,0);
    return `<div class="card section day-details"><div class="section-head"><h2>${esc(dateKey)} · ${niceHours(total)} / ${niceHours(max)}</h2></div>
        ${slots.map(s=>`<div class="session"><span>${esc(s.title)} · ${niceHours(s.hours)} ${s.manual?'(moved)':''}</span>
          <div class="task-actions"><button class="secondary" data-move="${s.taskId}" data-hours="${s.hours}" data-date="${esc(s.date)}">Move</button>
          ${s.pinId?`<button class="secondary" data-unpin="${s.pinId}">Auto-place</button>`:''}</div></div>`).join('')||'<p class="empty">No sessions on this day.</p>'}
      </div>`;
}
function weekdayForDate(iso){return (localDate(iso).getDay()+6)%7;}
function showDay(dateKey){
    const target=$('#calendar-day-details');
    if (target) {target.innerHTML=detailedDay(dateKey);target.scrollIntoView({block:'nearest',behavior:'smooth'});}
}
function moveForm(taskId,hours,oldDate) {
    const target=$('#calendar-day-details');
    if(!target)return;
    const info=tasks.find(t=>t.id===taskId);
    if(!info)return;
    target.innerHTML=`<div class="card section"><h2>Move ${esc(info.title)}</h2><p class="sub">Original: ${esc(oldDate)} · ${niceHours(hours)}. The rest of the timetable will be recalculated automatically.</p>
      <form id="move-session"><input type="hidden" name="taskId" value="${taskId}"><label>New date<input type="date" name="date" min="${futureDate(0)}" max="${esc(info.deadline)}" value="${esc(oldDate)}" required></label>
      <label>Hours<input type="number" name="hours" step="0.5" min="0.5" value="${Number(hours)}" required></label>
      <div class="form-actions"><button class="primary">Move session</button><button type="button" class="secondary" id="cancel-move">Cancel</button></div></form>
      <p class="sub" id="move-error"></p></div>`;
    $('#cancel-move').onclick=()=>showDay(oldDate);
    $('#move-session').onsubmit=async e=>{
        e.preventDefault();
        const f=new FormData(e.target);
        try {await api('schedule/move','POST',{taskId:Number(f.get('taskId')),date:f.get('date'),hours:Number(f.get('hours'))});await refresh();show('schedule');showDay(f.get('date'));}
        catch(error){$('#move-error').textContent=error.message;}
    };
}
async function unpin(pinId){
    try {await api('schedule/unpin','POST',{pinId:Number(pinId)});await refresh();show('schedule');}
    catch(e){alert(e.message);}
}

async function remindersPage() {
    const panel=$('#reminders-data');if(!panel)return;
    try {
        const {alerts}=await api('reminders');
        panel.innerHTML=alerts.map(a=>`<div class="session"><div><strong>${esc(a.title)}</strong><br><small>${esc(a.message)}</small></div><button class="secondary" data-dismiss="${esc(a.key)}">Dismiss</button></div>`).join('')||'<p class="empty">No active reminders. You are up to date.</p>';
    } catch(e) {panel.textContent=e.message;}
}
async function dismissReminder(key) {
    try { await api('reminders/dismiss','POST',{key});await remindersPage(); }
    catch(e) {alert(e.message);}
}

// Event delegation keeps handlers working after page redraws.
document.addEventListener('click',e=>{
    const day=e.target.closest('[data-calendar-day]');
    if(day)showDay(day.dataset.calendarDay);
    const move=e.target.closest('[data-move]');
    if(move)moveForm(Number(move.dataset.move),Number(move.dataset.hours),move.dataset.date);
    const unpinButton=e.target.closest('[data-unpin]');
    if(unpinButton)unpin(Number(unpinButton.dataset.unpin));
    const dismiss=e.target.closest('[data-dismiss]');
    if(dismiss)dismissReminder(dismiss.dataset.dismiss);
});
