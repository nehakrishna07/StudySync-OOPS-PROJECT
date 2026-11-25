// Shared frontend code: login, navigation, API requests, and small UI helpers.
// The backend runs locally at http://127.0.0.1:8080.

const $ = selector => document.querySelector(selector);

// These values are shared with pages.js, tasks.js, and features.js.
let token = sessionStorage.getItem('studysync-token') || '';
let name = sessionStorage.getItem('studysync-name') || '';
let page = 'dashboard';
let tasks = [];
let availability = [2, 2, 2, 2, 2, 3, 3];
let analysis = {};
let weekOffset = 0;
let register = false;

// Make user-entered text safe before displaying it inside HTML strings.
function esc(value) {
    const chars = {
        '&': '&amp;', '<': '&lt;', '>': '&gt;',
        '"': '&quot;', "'": '&#39;'
    };
    return String(value ?? '').replace(/[&<>"']/g, char => chars[char]);
}

// All frontend-to-C++ requests pass through this helper.
async function api(path, method = 'GET', data) {
    const response = await fetch('/api/' + path, {
        method: method,
        headers: {
            'Content-Type': 'application/json',
            Authorization: 'Bearer ' + token
        },
        body: data ? JSON.stringify(data) : undefined
    });

    const result = await response.json();
    if (!response.ok) {
        throw Error(result.error || 'Request failed');
    }
    return result;
}

function setTheme(dark) {
    document.body.classList.toggle('dark', dark);
    localStorage.setItem('studysync-theme', dark ? 'dark' : 'light');
    $('#theme-toggle span').textContent = dark ? 'Light mode' : 'Dark mode';
    $('#header-theme').textContent = dark ? '☀' : '☾';
}

setTheme(localStorage.getItem('studysync-theme') === 'dark');
$('#theme-toggle').onclick = $('#header-theme').onclick = () => {
    setTheme(!document.body.classList.contains('dark'));
};

// Switch between the login and registration forms.
$('#auth-switch').onclick = () => {
    register = !register;
    $('#name-wrap').hidden = !register;
    $('#auth-heading').textContent = register ? 'Create your space.' : 'Welcome back.';
    $('#auth-submit').textContent = register ? 'Create account' : 'Sign in';
    $('#auth-switch').textContent = register
        ? 'Already have an account? Sign in'
        : 'New here? Create an account';
    $('#auth-error').textContent = '';
};

$('#auth-form').onsubmit = async event => {
    event.preventDefault();
    $('#auth-error').textContent = '';

    try {
        const email = $('#email').value;
        const password = $('#password').value;

        if (register) {
            await api('register', 'POST', {
                name: $('#name').value, email, password
            });
            register = false;
            $('#auth-switch').click();
            $('#auth-error').textContent = 'Account created. Please sign in.';
            return;
        }

        const result = await api('login', 'POST', { email, password });
        token = result.token;
        name = result.name;
        sessionStorage.setItem('studysync-token', token);
        sessionStorage.setItem('studysync-name', name);
        await boot();
    } catch (error) {
        $('#auth-error').textContent = error.message;
    }
};

$('#logout').onclick = async () => {
    try {
        await api('logout', 'POST');
    } catch {
        // Even if the request fails, clear the browser's saved login.
    }
    token = '';
    sessionStorage.removeItem('studysync-token');
    sessionStorage.removeItem('studysync-name');
    $('#shell').hidden = true;
    $('#auth').hidden = false;
};

$('#nav').onclick = event => {
    const button = event.target.closest('[data-page]');
    if (button) {
        show(button.dataset.page);
    }
};

// Reload the information used on the dashboard and other pages.
async function refresh() {
    const [taskResult, availabilityResult, planResult] = await Promise.all([
        api('tasks'), api('availability'), api('analysis')
    ]);
    tasks = taskResult.tasks;
    availability = availabilityResult.hours;
    analysis = planResult;
}

async function boot() {
    try {
        await refresh();
        $('#auth').hidden = true;
        $('#shell').hidden = false;
        $('#side-name').textContent = name;
        $('#avatar').textContent = $('#top-avatar').textContent =
            name.charAt(0).toUpperCase();
        $('#today').textContent = new Date().toLocaleDateString('en-GB', {
            day: 'numeric', month: 'short', year: 'numeric'
        });
        show('dashboard');
    } catch (error) {
        token = '';
        sessionStorage.removeItem('studysync-token');
        $('#auth-error').textContent = error.message;
    }
}

// Simple HTML helpers used by several pages.
function date(value) {
    return new Date(value + 'T12:00:00').toLocaleDateString('en-GB', {
        day: 'numeric', month: 'short'
    });
}

function status(task) {
    const pendingClass = task.status === 'Pending' ? 'pending' : '';
    return `<span class="pill ${pendingClass}">${esc(task.status)}</span>`;
}

function table(rows) {
    const taskRows = rows.map(task => `
        <tr>
            <td><strong>${esc(task.title)}</strong><small>${esc(task.subject)}</small></td>
            <td>${date(task.deadline)}</td>
            <td>${task.remaining}h</td>
            <td>${status(task)}</td>
        </tr>
    `).join('');

    return `<table>
        <thead><tr><th>Task</th><th>Deadline</th><th>Remaining</th><th>Status</th></tr></thead>
        <tbody>${taskRows}</tbody>
    </table>${rows.length ? '' : '<p class="empty">No tasks yet. Add one to get started.</p>'}`;
}

function pageTop(title, desc, button = '') {
    return `<div class="page-top"><div><h1>${title}</h1><p class="sub">${desc}</p></div>${button}</div>`;
}

function stat(label, value, note) {
    return `<div class="card stat"><span class="label">${label}</span><strong>${value}</strong><small>${note}</small></div>`;
}
