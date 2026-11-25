// PAGE RENDERING: Dashboard, workload, reminders and sidebar navigation.
// Render dashboard, task list, timetable, availability and reminders.
// Use the student's local computer time for the welcome message.
function getGreeting() {
    const hour = new Date().getHours();
    if (hour < 12) return 'Good morning';
    if (hour < 17) return 'Good afternoon';
    return 'Good evening';
}

function show(p) {
    page = p;
    document.querySelectorAll('nav button').forEach(b => b.classList.toggle('active', b.dataset.page === p));
    $('#breadcrumb').textContent = p.toUpperCase().replace('-', ' ');
    const active = tasks.filter(t => t.status !== 'Completed'), required = active.reduce((a, t) => a + t.remaining, 0), completed = tasks.length - active.length, cap = availability.reduce((a, b) => a + b, 0), util = cap ? Math.round(required / cap * 100) : required ? 101 : 0;
    let html = '';
    if (p === 'dashboard') {
        html = pageTop(`${getGreeting()}, ${esc(name.split(' ')[0])} ✦`, 'Here’s what your academic week looks like.', `<button class="primary" id="new-task">+ New task</button>`) + `<div class="grid3">${stat('Active tasks', active.length.toString().padStart(2, '0'), 'Across your subjects')}${stat('Work remaining', required + 'h', 'Estimated study time')}${stat('Completed', completed.toString().padStart(2, '0'), 'Tasks finished')}</div><div class="card section"><div class="section-head"><h2>Task Overview</h2><button class="link" data-go="tasks">View all tasks ↗</button></div>${table(tasks.slice(0, 5))}</div><div class="grid2 section"><div class="card"><div class="section-head"><h2>Study Timeline</h2><button class="link" data-go="schedule">Full schedule ↗</button></div>${analysis.sessions.slice(0, 4).map(s => `<div class="session"><span>${esc(s.title)}<br><small>${date(s.date)}</small></span><strong>${s.hours}h</strong></div>`).join('') || '<p class="muted">No sessions scheduled yet.</p>'}</div><div class="card"><h2>Smart Insights</h2><p class="sub">Weekly capacity</p><h1 style="margin-top:16px">${util}%</h1><div class="bar"><div style="width:${Math.min(util, 100)}%"></div></div><p class="muted">${required}h required / ${cap}h available per week</p><div class="notice">${analysis.unallocated > 0 ? `${analysis.unallocated.toFixed(1)} hours could not be scheduled before their deadlines.` : active.length ? 'Your pending tasks fit into the current 30-day planning window.' : 'Add tasks to receive planning insights.'}</div></div></div>`;
    }
    if (p === 'tasks') {
        html = pageTop('My Tasks', 'Keep track of assignments, exams and projects.', `<button class="primary" id="new-task">+ Add task</button>`) + `<div class="card"><div class="section-head"><h2>All academic tasks</h2><span class="muted">${tasks.length} total</span></div><div style="overflow:auto"><table><thead><tr><th>Task</th><th>Due</th><th>Remaining</th><th>Priority</th><th>Status / Actions</th></tr></thead><tbody>${tasks.map(t => `<tr><td><strong>${esc(t.title)}</strong><small>${esc(t.subject)} · ${esc(t.category)}</small></td><td>${date(t.deadline)}</td><td>${t.remaining}h</td><td>${esc(t.priority)}</td><td><div class="task-actions"><button class="secondary" data-complete="${t.id}">${t.status === 'Completed' ? 'Reopen' : 'Complete'}</button><button class="secondary" data-edit="${t.id}">Edit</button><button class="secondary" data-progress="${t.id}">Progress</button><button class="secondary" data-delete="${t.id}">Delete</button></div></td></tr>`).join('')}</tbody></table></div>${tasks.length ? '' : '<p class="empty">Your task list is empty.</p>'}</div><div id="task-form"></div>`;
    }
    if (p === 'schedule') {
        html = pageTop('Smart Schedule', 'A balanced weekly view of your deadline-aware study sessions.') + calendarWeek() + explanationPanel();
    }
    if (p === 'availability') {
        html = pageTop('Study Availability', 'Set the hours you can realistically study each weekday.') + `<div class="card"><form id="availability-form">${['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday'].map((d, i) => `<div class="day-row"><strong>${d}</strong><input aria-label="${d} hours" type="number" min="0" max="24" step="0.5" name="day${i}" value="${availability[i]}" required></div>`).join('')}<p class="sub">Changes will automatically update your next schedule.</p><button class="primary" style="margin-top:20px">Save availability</button></form></div>`;
    }
    if (p === 'workload') {
        html = pageTop('Workload Analysis', 'A clearer view of your academic capacity.') + `<div class="grid3">${stat('Required hours', required + 'h', 'Pending work')}${stat('Weekly availability', cap + 'h', 'Your study capacity')}${stat('Unallocated hours', analysis.unallocated.toFixed(1) + 'h', 'Within the 30-day plan')}</div><div class="card section"><h2>Weekly utilisation</h2><h1 style="margin-top:18px">${util}%</h1><div class="bar"><div style="width:${Math.min(100, util)}%"></div></div><p class="muted">${util > 100 ? 'Overloaded' : util >= 80 ? 'High' : util >= 60 ? 'Moderate' : 'Low'} weekly workload (total pending hours ÷ one week of availability).</p><p class="notice">Feasibility is checked separately against actual deadlines and daily capacity. ${analysis.risks.length ? analysis.risks.length + ' task(s) need attention.' : 'No unallocated work in the planning window.'}</p></div>`;
    }
    if (p === 'reminders') {
        html = pageTop('Reminders', 'Due dates, insufficient-time alerts and dismissible notices.') +
            `<div class="card section"><h2>Planning alerts</h2><div id="reminders-data">Loading reminders...</div></div>`;
    }
    if (p === 'simulator') html = simulatorPage();
    if (p === 'timer') html = timerPage();
    if (p === 'tracking') html = trackingPage();
    $('#content').innerHTML = `<div class="page">${html}</div>`;
    if (p === 'schedule') {
        const bind = (id, fn) => { const b = $('#' + id); if (b)
            b.onclick = fn; };
        bind('calendar-prev', () => { weekOffset = Math.max(-1, weekOffset - 1); show('schedule'); });
        bind('calendar-today', () => { weekOffset = 0; show('schedule'); });
        bind('calendar-next', () => { weekOffset = Math.min(4, weekOffset + 1); show('schedule'); });
        $('#calendar-prev').disabled = weekOffset <= -1;
        $('#calendar-next').disabled = weekOffset >= 4;
    }
    $('#content').querySelectorAll('[data-go]').forEach(b => b.onclick = () => show(b.dataset.go));
    $('#new-task')?.addEventListener('click', () => { show('tasks'); openTaskForm(); });
    $('#content').querySelectorAll('[data-complete]').forEach(b => b.onclick = async () => { const t = tasks.find(x => x.id == b.dataset.complete); await updateTask(t, t.status === 'Completed' ? 'Pending' : 'Completed', t.status === 'Completed' ? t.estimated : 0); });
    $('#content').querySelectorAll('[data-edit]').forEach(b => b.onclick = () => openTaskForm(tasks.find(t => t.id == b.dataset.edit)));
    $('#content').querySelectorAll('[data-progress]').forEach(b => b.onclick = async () => { const t = tasks.find(x => x.id == b.dataset.progress); const n = prompt('Remaining hours for ' + t.title, t.remaining); if (n !== null && n.trim() !== '' && Number(n) >= 0 && Number(n) <= t.estimated)
        await updateTask(t, Number(n) === 0 ? 'Completed' : 'In Progress', Number(n)); });
    $('#content').querySelectorAll('[data-delete]').forEach(b => b.onclick = async () => { if (confirm('Delete this task?')) {
        try {
            await api('tasks/' + b.dataset.delete, 'DELETE');
            await refresh();
            show('tasks');
        }
        catch (e) {
            alert(e.message);
        }
    } });
    $('#availability-form')?.addEventListener('submit', saveAvailability);
    $('#simulate-form')?.addEventListener('submit', simulateSubmit);
    $('#manual-log')?.addEventListener('submit', manualLog);
    if(p === 'timer') focusBind();
    if(p === 'tracking') loadTracking();
    if(p === 'reminders') remindersPage();
}
