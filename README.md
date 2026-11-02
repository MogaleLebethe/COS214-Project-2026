# COS214 Project 2026 - TIVIDY Workflow Board

TIVIDY is a workflow-management application that exposes a browser-based board for creating and tracking projects, tasks, dependencies, and workflow progress. The project combines a C++ workflow engine with a web front-end served by the application runtime.

## Project overview

The project is organised around these areas:

- `app/` – application entry point for the embedded web server
- `gui/` – GUI and API controller code
- `src/` – workflow engine and domain logic
- `web/` – browser UI files (HTML, CSS, JavaScript)
- `tests/` – automated tests for the workflow engine and commands
- `docs/` – project reference material and specification documents

## Requirements

To run the project you need:

- Docker 24+ (recommended option)
- A modern browser such as Chrome, Edge, Firefox, or Safari
- Optional native development tools: build-essential, make, g++, and the C++ libraries required by the project

## Quick start with Docker

From the repository root, build the container image:

```bash
docker build -t tividy-gui .
```

Start the application:

```bash
docker run -d --name tividy-gui -p 8080:8080 tividy-gui
```

Once the container is running, open the browser at:

```text
http://localhost:8080/
```

Useful Docker commands:

```bash
docker ps

docker logs tividy-gui

docker stop tividy-gui

docker rm -f tividy-gui
```

## Running the application locally

If you have already built the project binary, run it directly from the repository root:

```bash
./tividy_gui --port 8080 --web ./web
```

This starts the server and serves the web interface from the `web/` directory. The browser UI will then be available at `http://localhost:8080/`.

If the binary has not been built yet, build it using the project’s build process before running the command above.

## Development notes

- The server exposes a JSON API under `/api/*` and serves the browser UI through the `web/` directory.
- The workflow engine manages projects, tasks, dependency rules, action execution, imports/exports, and undo/redo support.
- The web app communicates with the backend through the API to update the board and project state.

## Troubleshooting

- If the app does not start, check the container logs:

```bash
docker logs tividy-gui
```

- If the port is already in use, change the exposed port in the run command, for example:

```bash
docker run -d --name tividy-gui -p 8081:8080 tividy-gui
```

- If you are running locally, confirm that the `web/` directory exists and that the built binary is present in the repository root.

## Example workflow

1. Start the TIVIDY server.
2. Open the app in a browser.
3. Create or import a project.
4. Add tasks and dependencies.
5. Execute workflow actions and monitor progress.
6. Export or clone the project as needed.

This should give you a complete starting point for working with the TIVIDY workflow board.