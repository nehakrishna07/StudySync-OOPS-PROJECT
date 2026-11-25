// Monday-to-Sunday timetable. C++ decides the sessions; JS only displays them.
// We use local noon to avoid date changes caused by timezone conversions.
function localDate(iso) {
    const [year, month, day] = iso.split('-').map(Number);
    return new Date(year, month - 1, day, 12);
}

function isoDay(dateValue) {
    const year = dateValue.getFullYear();
    const month = String(dateValue.getMonth() + 1).padStart(2, '0');
    const day = String(dateValue.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
}

function mondayOf(dateValue) {
    const monday = new Date(dateValue);
    monday.setHours(12, 0, 0, 0);
    monday.setDate(monday.getDate() - (monday.getDay() + 6) % 7);
    return monday;
}

function shiftDate(dateValue, days) {
    const result = new Date(dateValue);
    result.setDate(result.getDate() + days);
    return result;
}

function calendarWeek() {
    const start = shiftDate(mondayOf(new Date()), 7 * weekOffset);
    const end = shiftDate(start, 6);

    // Group the scheduled sessions by date.
    const sessionsByDate = new Map();
    for (const session of analysis.sessions || []) {
        if (!sessionsByDate.has(session.date)) {
            sessionsByDate.set(session.date, []);
        }
        sessionsByDate.get(session.date).push(session);
    }

    // Make one column for each day of the week.
    const columns = Array.from({ length: 7 }, (_, index) => {
        const day = shiftDate(start, index);
        const dayKey = isoDay(day);
        const sessions = sessionsByDate.get(dayKey) || [];
        const available = Number(availability[index] || 0);
        const allocated = sessions.reduce((sum, session) => sum + Number(session.hours), 0);
        const free = Math.max(0, available - allocated);
        const percent = available ? Math.min(100, allocated / available * 100) : 0;
        const isToday = dayKey === isoDay(new Date());

        const sessionCards = sessions.map(session => `
            <button type="button" class="calendar-task day-open"
                    data-calendar-day="${esc(session.date)}">
                <strong>${esc(session.title)}</strong>
                <span>${Number(session.hours).toFixed(1).replace(/\.0$/, '')}h allocated</span>
            </button>
        `).join('');

        return `
            <div class="calendar-day ${isToday ? 'is-today' : ''}">
                <button type="button" class="calendar-day-head day-open"
                        data-calendar-day="${dayKey}" title="View sessions for this day">
                    <span class="calendar-weekday">${day.toLocaleDateString('en-GB', { weekday: 'short' })}</span>
                    <span class="calendar-date">${day.getDate()}</span>
                </button>
                <div class="calendar-capacity">
                    <span>${allocated.toFixed(1).replace(/\.0$/, '')} / ${available.toFixed(1).replace(/\.0$/, '')}h</span>
                    <div class="calendar-meter"><div style="width:${percent}%"></div></div>
                </div>
                <div class="calendar-task-list">
                    ${sessionCards || '<span class="calendar-empty">No sessions</span>'}
                </div>
                <div class="calendar-free">${free.toFixed(1).replace(/\.0$/, '')}h free</div>
            </div>`;
    });

    const thisWeek = (analysis.sessions || []).filter(session =>
        session.date >= isoDay(start) && session.date <= isoDay(end)
    );
    const total = thisWeek.reduce((sum, session) => sum + Number(session.hours), 0);
    const weeklyCapacity = availability.reduce((sum, hours) => sum + Number(hours), 0);
    const title = `${start.toLocaleDateString('en-GB', { day: 'numeric', month: 'short' })} – ${end.toLocaleDateString('en-GB', { day: 'numeric', month: 'short', year: 'numeric' })}`;
    const risks = analysis.risks || [];

    // Keep the same IDs and CSS class names so other features still work.
    return `
        <div class="card timetable">
            <div class="timetable-toolbar">
                <div>
                    <span class="eyebrow">WEEKLY TIMETABLE</span>
                    <h2>${title}</h2>
                    <p class="sub">${total.toFixed(1).replace(/\.0$/, '')}h scheduled this week · ${weeklyCapacity.toFixed(1).replace(/\.0$/, '')}h weekly capacity</p>
                </div>
                <div class="calendar-controls">
                    <button class="secondary" id="calendar-prev" aria-label="Previous week">←</button>
                    <button class="secondary" id="calendar-today">This week</button>
                    <button class="secondary" id="calendar-next" aria-label="Next week">→</button>
                </div>
            </div>
            <div class="calendar-scroll"><div class="calendar-grid">${columns.join('')}</div></div>
            <div class="calendar-legend">
                <span><i class="legend-sage"></i> Scheduled session</span>
                <span><i class="legend-border"></i> Available study hours</span>
                <span>Calendar shows the next 30 days of generated sessions</span>
            </div>
        </div>
        <div id="calendar-day-details"></div>
        <div class="schedule-summary">
            <div class="card schedule-summary-card"><span class="label">Allocated in 30 days</span><strong>${Number(analysis.allocated || 0).toFixed(1)}h</strong></div>
            <div class="card schedule-summary-card"><span class="label">Unallocated work</span><strong class="${Number(analysis.unallocated || 0) > 0 ? 'warning-text' : ''}">${Number(analysis.unallocated || 0).toFixed(1)}h</strong></div>
            <div class="card schedule-summary-card"><span class="label">Tasks needing attention</span><strong>${risks.length}</strong></div>
        </div>
        ${risks.length ? `
            <div class="card section">
                <h2>Work that could not be scheduled</h2>
                <p class="sub">These hours are not included in the calendar above.</p>
                ${risks.map(risk => `<p class="notice"><strong>${esc(risk.title)}</strong> — ${Number(risk.unallocated).toFixed(1)}h unallocated. ${esc(risk.reason)}</p>`).join('')}
            </div>
        ` : ''}`;
}
