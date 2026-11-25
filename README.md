# StudySync

**A smart academic planner and workload management web application built with C++17.**

StudySync helps students organise assignments, projects, exams and revision, plan their available study hours, and identify when upcoming deadlines may be difficult to meet. It combines a browser-based interface with a C++ backend and SQLite database.

> **Project type:** C++ Object-Oriented Programming (OOP) mini-project  
> **Platform:** Local web application (runs on your computer)  
> **No Qt, Node.js or frontend build system required.**

## Features

- **User accounts:** Registration and login with separate tasks and study data for each account.
- **Task management:** Add, view, edit and delete academic tasks with categories, priorities, difficulty levels, deadlines and estimated hours.
- **Smart scheduling:** Generate a study plan based on deadlines, remaining task hours and weekly availability.
- **Weekly availability:** Set the amount of time available for studying on different days.
- **Workload analysis:** Review workload, scheduling feasibility and unallocated study hours.
- **Interactive calendar:** View scheduled sessions and manually move or release sessions, subject to deadline and capacity checks.
- **What-if Simulator:** Add a hypothetical task to see how it would affect the schedule, without saving it.
- **Focus Timer:** Study using a 25-minute timer and 5-minute break option; log completed study time manually.
- **Time tracking:** Compare estimated work with actual study time and track remaining hours.
- **Smart reminders:** See upcoming deadlines, overdue tasks and scheduling warnings; dismiss reminders.
- **Explainable scheduling:** View explanations for task ordering and hours that could not be allocated.
- **Light and dark themes:** Includes a time-aware dashboard greeting.

## Technology stack

| Layer | Technology |
|---|---|
| Backend | C++17 |
| Frontend | HTML, CSS, vanilla JavaScript |
| Database | SQLite3 |
| Server | Local C++ HTTP server |
| Build system | CMake |
| Windows development | VS Code, MSYS2 UCRT64, GCC and Ninja |

The app uses **no Qt framework** and does not require npm.

## How the application works

```text
Browser (HTML / CSS / JavaScript)
               |
         HTTP + JSON
               |
       Local C++ server
               |
       App request handler
          /         \
   TaskManager    Scheduler
          \         /
          SQLite database
```

1. The user interacts with the website.
2. JavaScript sends a request to the local C++ server.
3. The backend validates the request and reads or updates the database.
4. The scheduler generates a deadline-aware study plan when needed.
5. The browser displays the response.

## OOP concepts demonstrated

| Concept | Implementation |
|---|---|
| Classes and objects | `Task`, `TaskManager`, `Scheduler`, `Database`, etc. |
| Encapsulation | Private task fields accessed through getters and validated setters |
| Parameterised constructors | Initialising academic task details |
| Inheritance | `Assignment`, `Project`, `LabWork`, `Exam` and `Revision` derive from `Task` |
| Abstraction | Abstract `Task` base class |
| Runtime polymorphism | Virtual functions used through base-class pointers |
| Function overriding | Task-type-specific implementations |
| Function overloading | `findTask()` by task ID or title |
| Operator overloading | `operator>` compares task scores |
| STL and templates | Containers, smart pointers, sorting utilities |
| Exception handling | Error handling for invalid inputs and database operations |

## Project structure

```text
StudySync/
├── backend/
│   ├── main.cpp
│   ├── models/
│   │   ├── Task.h
│   │   ├── Task.cpp
│   │   └── Json.h
│   ├── services/
│   │   ├── App.h / App.cpp
│   │   ├── TaskManager.h / TaskManager.cpp
│   │   └── Scheduler.h / Scheduler.cpp
│   ├── database/
│   │   └── Database.h / Database.cpp
│   ├── server/
│   │   └── HttpServer.h / HttpServer.cpp
│   └── utils/
│       └── Common.h
├── frontend/
│   ├── index.html
│   ├── css/style.css
│   └── js/
│       ├── app.js
│       ├── pages.js
│       ├── tasks.js
│       ├── calendar.js
│       └── features.js
├── docs/
├── CMakeLists.txt
└── README.md
```

## Getting started (Windows 11)

### 1. Requirements

Install:

- [Visual Studio Code](https://code.visualstudio.com/)
- [MSYS2](https://www.msys2.org/) with the **UCRT64** environment
- Git (only needed for cloning and uploading to GitHub)

Open **MSYS2 UCRT64**, then install the compiler, CMake, Ninja and SQLite:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sqlite3
```

### 2. Get the project

Download the repository as a ZIP from GitHub **or** clone it:

```bash
git clone https://github.com/YOUR_USERNAME/StudySync.git
cd StudySync
```

Replace `YOUR_USERNAME` with the actual GitHub username. Open the project folder (the one containing `CMakeLists.txt`) in VS Code.

### 3. Build in VS Code

Open **Terminal → New Terminal** and use **PowerShell**.

First, add the MSYS2 UCRT64 programs to your terminal's PATH:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
```

Configure and build:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
```

**If you removed the `tests/` folder:** also remove its `StudySyncCoreTests` target and related test commands from `CMakeLists.txt`, or configure with `-DBUILD_TESTING=OFF`:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 -DBUILD_TESTING=OFF
```

If CMake reports that the existing `build/` folder was configured with a different generator, delete **only** the generated `build/` folder and run the configuration command again.

### 4. Run StudySync

From the project root in the same terminal:

```powershell
.\build\StudySyncWeb.exe
```

Open **http://127.0.0.1:8080** in your browser.

Keep the terminal open while using the website. Press `Ctrl+C` to stop the server.

### 5. First use

1. Register an account and log in.
2. Set your weekly study availability.
3. Add academic tasks, deadlines and estimated hours.
4. Open **Smart Schedule** to generate a study plan.
5. Explore the calendar, simulator, timer, tracking and reminders.

## Scheduling approach

StudySync uses an explainable, **deadline-first greedy scheduling algorithm**:

1. Collect unfinished tasks with remaining work.
2. Prioritise earlier deadlines and use task scores when needed.
3. Calculate daily capacity from weekly study availability.
4. Respect manually reserved sessions.
5. Allocate remaining work to available days before deadlines.
6. Report tasks or hours that do not fit.

The scheduler plans within a **30-day window**. This is a heuristic academic planner, not a guarantee that every task can be finished.
<!--
## Screenshots

Add screenshots captured from **your running application** to a `screenshots/` folder, then uncomment these lines:


### Light mode
![StudySync light mode](screenshots/light-mode.png)

### Dark mode
![StudySync dark mode](screenshots/dark-mode.png)
-->

## Notes and limitations

- StudySync runs locally at `127.0.0.1:8080` and is **not designed for public internet deployment**.
- The Focus Timer only saves study time when the user explicitly logs it; refreshing the page can reset an unlogged timer.
- Manually moved calendar sessions are reserved for a selected day; the feature is not drag-and-drop.
- The What-if Simulator does **not** save hypothetical tasks.
- Login sessions are held in memory, so restarting the server may require logging in again.
- The SQLite database is stored under `data/studysync.db`, relative to the folder where the server starts. Keep it out of version control.

## Troubleshooting

**`cmake` or `g++` not recognised:** Confirm MSYS2 UCRT64 is installed and run the PATH command in the current terminal.

**CMake says `tests/core_tests.cpp` is missing:** Configure with `-DBUILD_TESTING=OFF`, or remove the test target from `CMakeLists.txt`.

**CMake generator mismatch:** Remove the generated `build/` directory and configure again using Ninja.

**Website not opening:** Make sure `StudySyncWeb.exe` is running and visit `http://127.0.0.1:8080`.

**Port already in use:** Stop the earlier StudySync process before starting another instance.

<!--
## Documentation

See `docs/` for additional code explanations, OOP concept notes, refactoring information and viva preparation material.
-->
---

**StudySync** — *Plan smarter. Study better.*
