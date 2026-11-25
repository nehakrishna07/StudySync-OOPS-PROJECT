# StudySync — Student-Friendly OOP Refactor

**Same working StudySync web application; clearer C++ class organisation.**

This edition retains separate `.h` and `.cpp` files for the main C++ classes, makes `Task` fields private with validated setters and a parameterised constructor, and breaks the large request handler and scheduler preparation into named steps. The existing API routes, database schema and six added features are retained. `findTask` demonstrates function overloading.

**Latest cleanup:** `App::taskRoutes()` now delegates to `listTasks()`, `addTask()`, `editTask()`, `deleteTask()` and `updateTaskProgress()`; availability GET/PUT use `getAvailability()` and `saveAvailability()`. The scheduler delegates pinned sessions, automatic allocation, explanations and sorting to small helpers. These changes are organisational; endpoint names, storage and scheduling rules are unchanged.

**Latest readability update:** calendar session moves and study-time logging are now broken into small, easy-to-read methods. See `docs/FINAL_READABILITY_CHANGES.md`.

**New:** the dashboard greeting follows your browser’s local time (morning, afternoon or evening).

Read **`docs/OOP_CONCEPTS_AND_CODE_GUIDE.md`** for the exact OOP concepts, examples and limitations.

**Do not overwrite an existing working installation.** Extract into a new folder. Copy your backed-up `data/studysync.db` only if you want to preserve tasks/accounts, and keep the old version until the new one works on your Windows PC.

## Run in VS Code (Windows 11, MSYS2 UCRT64)

Open the project root folder, not `build`, and open a PowerShell terminal:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
.\build\StudySyncWeb.exe
```

Then go to http://127.0.0.1:8080 in Chrome. Keep the server terminal open.

If an old `build` folder uses MinGW Makefiles instead of Ninja, delete **only** the generated `build` directory (after checking no database was placed there), then reconfigure. The SQLite database normally lives at `data/studysync.db` relative to the terminal's working directory.

## Test (optional)

```powershell
ctest --test-dir build --output-on-failure
```

# StudySync — Enhanced (no Qt)

A **local browser-based academic planner** for a C++17 OOP mini-project.

- **Frontend:** HTML, CSS, vanilla JavaScript (no npm)
- **Backend:** C++17 local HTTP server and a deadline-first scheduling class
- **Storage:** SQLite3 using prepared statements
- **Design:** ivory/sage-green light mode and forest-charcoal/sage dark mode
- **Development:** VS Code + MSYS2 UCRT64 + CMake/Ninja on Windows 11
- **No Qt dependencies.**

## All six new features

1. **Interactive weekly calendar:** select a day or task block; choose **Move** to reserve a session on another day. It cannot be placed after the task deadline or beyond daily capacity. **Auto-place** removes a manual reservation. Any other sessions are recalculated from the remaining capacity. This is a controlled rescheduling action, not drag-and-drop.
2. **What-if Simulator:** enter a hypothetical task to compare unallocated work **before/after**. This calls C++ scheduling logic **without saving** the hypothetical task.
3. **Focus Timer:** 25-minute study timer, pause/reset, and a 5-minute break button. **Finish & log** writes completed minutes to SQLite, and changes the task's remaining work. It never records break minutes.
4. **Estimated vs Actual:** open **Time Tracking** to manually log study minutes, view each task's original estimate, cumulative logged time and remaining estimated hours. Recent sessions show the last 100 entries; totals include all sessions.
5. **Smart Reminders:** deadlines coming within three days, overdue tasks, and unallocated work warnings. Each reminder can be dismissed; the dismissal is tied to your account in SQLite.
6. **Explainable Scheduling:** open Smart Schedule → **Why this schedule?** to see why a task was ordered and what couldn't fit.

## Install (once)

Open **MSYS2 UCRT64** and run:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sqlite3
```

## Run in VS Code (PowerShell)

Extract this ZIP and open the **StudySync_Enhanced** folder (the one with `CMakeLists.txt`).

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
.\build\StudySyncWeb.exe
```

With the server running, open **http://127.0.0.1:8080** in Chrome. Keep the terminal open. Press `Ctrl+C` to stop the server. Refresh with `Ctrl+Shift+R` after frontend changes.

**Already built using MinGW Makefiles?** CMake caches the generator in `build/`. Delete only that old `build/` directory before configuring again, e.g. `Remove-Item -Recurse -Force build` in PowerShell. This does **not** remove the `data/` directory.

## Keep your existing accounts and tasks

1. Stop your previous server.
2. **Back up the complete previous StudySync folder** and specifically `data/studysync.db`.
3. Extract this ZIP into a **new** `StudySync_Enhanced` folder. Do not mix old frontend/C++ files with the new ones.
4. Copy your old `data/studysync.db` into the new project's `data/` folder **before the first start**.
5. Build and launch this new project **from its root folder**, using the commands above.

The new version creates three **additional tables** in the existing SQLite database (`time_logs`, `pinned_sessions`, `dismissed_alerts`); it doesn't replace the original Users/Tasks/Availability tables. The database path depends on the *current working directory*, so always launch from the project root. Sessions/tokens are in memory and require login after a server restart.

## What the new files do

| File | Responsibility |
|---|---|
| `backend/models/Task.h` | Abstract Task, derived task types, priority and generic sorting |
| `backend/services/Scheduler.h` | Deadline-aware greedy scheduling; manual session reservations and explanations |
| `backend/services/App.h` | API endpoints, task validation, simulation, time logs, reminders |
| `backend/services/TaskManager.h` | Converts SQLite rows into polymorphic Task objects |
| `backend/database/Database.h` | SQLite schema, migration-safe additive tables and prepared statements |
| `backend/server/HttpServer.h` | Local HTTP server, JSON requests and static files |
| `frontend/js/pages.js` | Core page rendering |
| `frontend/js/calendar.js` | Seven-day timetable |
| `frontend/js/features.js` | New simulator, timer, tracking, reminders and day controls |
| `frontend/js/tasks.js` | Existing task forms and availability |
| `frontend/css/style.css` | Light/dark theme and layout |
| `docs/VIVA_GUIDE.md` | Student-friendly explanation and OOP mapping |

## Tests

```powershell
ctest --test-dir build --output-on-failure
```

The core C++ test covers deadline scheduling, pinning, completed-task exclusion and OOP foundations. Optional API integration tests require Python and a running built executable (`python tests/integration_test.py build/StudySyncWeb.exe`); the integration script starts a temporary server and creates a temporary DB, not your real data. `tests/ui_smoke.py` is an optional visual test that requires Python Playwright and Chromium (not needed to run StudySync).

## Important limits

- The scheduler uses a **30-day planning window**, with an explainable greedy algorithm. Tasks later than the window may still have unallocated hours until a later planning period.
- Manual scheduling **anchors** hours on your chosen day and recalculates the rest; it doesn't perform mouse drag-and-drop or guarantee the previous auto-generated day remains empty.
- The weekly utilisation percentage divides *all remaining estimated work* by *one week's capacity*, as a simple workload indicator. Per-deadline feasibility comes from the actual date-based scheduler.
- Time logging subtracts actual study minutes from estimated remaining hours. These are two different measures; actual hours can exceed the initial estimate.
- The timer runs in your browser and the session is only stored after you press **Finish & log**. Closing or refreshing the tab without logging loses its timer state.
- This is an educational **localhost** app, not a hardened publicly deployed service. Do not expose its port to the internet.

## Student-friendly readability update

This edition retains the same functionality as the six-feature StudySync version, but
makes the most important C++ and frontend files easier to read, with plain names,
step-by-step loops, consistent indentation, and comments describing *why* the code
works. No new dependencies or Qt components were introduced.

**Learn the code:** see [`docs/CODE_EXPLANATION.md`](docs/CODE_EXPLANATION.md) and
[`docs/READ_THIS_FIRST.md`](docs/READ_THIS_FIRST.md). The existing
[`docs/VIVA_GUIDE.md`](docs/VIVA_GUIDE.md) is included too.


## Final readability refactor (November 2026)

The following larger functions were separated without changing the database schema or JSON API:

- `HttpServer.cpp`: `serve()` now delegates to `readRequest()`, `handleApiRequest()` and `serveStaticFile()`.
- `App.cpp`: `registerUser()` delegates validation and insertion to `validRegistration()` and `createUser()`.
- `calendarRoutes()` delegates to `moveSession()` and `unpinSession()`.
- `timeRoutes()` delegates to `getTimeLogs()` and `saveTimeLog()`.
- `alerts()` delegates to `getWorkloadAlerts()` and `getDeadlineAlerts()`, then filters dismissed alerts.

These are internal code-organisation changes. Your dynamic greeting and existing features remain unchanged.
