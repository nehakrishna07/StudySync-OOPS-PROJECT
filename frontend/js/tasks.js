// Task forms and the availability form. The backend validates all changes.

async function updateTask(task, status, remaining) {
    try {
        await api('tasks/' + task.id, 'PUT', { status, remaining });
        await refresh();
        show(page);
    } catch (error) {
        alert(error.message);
    }
}

async function saveAvailability(event) {
    event.preventDefault();
    const hours = [];
    for (let day = 0; day < 7; day++) {
        hours.push(Number(event.target.elements['day' + day].value));
    }

    try {
        await api('availability', 'PUT', { hours });
        await refresh();
        show('availability');
        alert('Availability saved. Schedule recalculated.');
    } catch (error) {
        alert(error.message);
    }
}

// Build a dropdown for category, difficulty, or priority.
function taskOptions(options, selected) {
    return options.map(value => {
        const isSelected = value === selected ? 'selected' : '';
        return `<option ${isSelected}>${value}</option>`;
    }).join('');
}

function openTaskForm(existing = null) {
    const container = $('#task-form');
    const editing = existing !== null;
    const category = existing?.category || 'Assignment';
    const difficulty = existing?.difficulty || 'Medium';
    const priority = existing?.priority || 'Medium';
    const minimumDate = new Date().toLocaleDateString('en-CA');

    container.innerHTML = `
        <div class="card section">
            <h2>${editing ? 'Edit task' : 'Add a new task'}</h2>
            <form id="add-form" style="margin-top:20px">
                <div class="form-grid">
                    <label>Task title
                        <input name="title" required placeholder="e.g. OOP Mini Project"
                               value="${esc(existing?.title || '')}">
                    </label>
                    <label>Subject
                        <input name="subject" required placeholder="Object Oriented Programming"
                               value="${esc(existing?.subject || '')}">
                    </label>
                    <label>Category
                        <select name="category">
                            ${taskOptions(['Assignment', 'Project', 'Lab Work', 'Exam', 'Revision'], category)}
                        </select>
                    </label>
                    <label>Deadline
                        <input name="deadline" type="date" required min="${minimumDate}"
                               value="${esc(existing?.deadline || '')}">
                    </label>
                    <label>Estimated hours
                        <input name="estimated" type="number" min="0.5" step="0.5"
                               value="${existing?.estimated || 2}" required>
                    </label>
                    <label>Difficulty
                        <select name="difficulty">
                            ${taskOptions(['Easy', 'Medium', 'Hard'], difficulty)}
                        </select>
                    </label>
                    <label>Priority
                        <select name="priority">
                            ${taskOptions(['Low', 'Medium', 'High'], priority)}
                        </select>
                    </label>
                </div>
                <label>Description (optional)
                    <textarea name="description">${esc(existing?.description || '')}</textarea>
                </label>
                <p class="error" id="task-error"></p>
                <div class="form-actions">
                    <button type="button" class="secondary" id="cancel-task">Cancel</button>
                    <button class="primary">${editing ? 'Save changes' : 'Create task'}</button>
                </div>
            </form>
        </div>`;

    container.scrollIntoView({ behavior: 'smooth' });
    $('#cancel-task').onclick = () => { container.innerHTML = ''; };

    $('#add-form').onsubmit = async event => {
        event.preventDefault();

        const form = new FormData(event.target);
        const data = Object.fromEntries(form.entries());
        data.estimated = Number(data.estimated);

        // Editing keeps the progress already made on the task.
        if (editing) {
            data.remaining = Math.min(existing.remaining, data.estimated);
            data.status = existing.status;
        }

        try {
            const endpoint = editing ? 'tasks/' + existing.id : 'tasks';
            const method = editing ? 'PUT' : 'POST';
            await api(endpoint, method, data);
            await refresh();
            show('tasks');

            if (analysis.unallocated > 0) {
                alert('Task saved. StudySync detected ' +
                      analysis.unallocated.toFixed(1) +
                      ' unallocated hours across your tasks. Check Smart Schedule.');
            }
        } catch (error) {
            $('#task-error').textContent = error.message;
        }
    };
}

// Restore the signed-in dashboard after a browser refresh.
if (token) {
    boot();
}
