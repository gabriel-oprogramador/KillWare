# KillWare

## 🚧 In Development

![Windows](https://img.shields.io/badge/Windows-green)
![Linux](https://img.shields.io/badge/Linux-green)
![Web](https://img.shields.io/badge/Web-lightgrey)

---

## 🌎 Official Links

[![YouTube](https://img.shields.io/badge/YouTube-Channel-yellow?logo=youtube)](https://www.youtube.com/@gabriel-oprogramador)
[![Itch.io](https://img.shields.io/badge/Itch.io-Page-yellow?logo=itch.io\&logoColor=white)](https://gabriel-oprogramador.itch.io/)

---

## Overview

**KillWare** is a **fast-paced FPS runner** set inside the **Intra-Net**, a virtual network where consciousnesses connect.

You play as a hunter of **Wares** — run, shoot, dodge, and clean infected servers before being expelled from the network.

---

## Project

The game is developed in **pure C**, using a custom low-level foundation focused on:

* **ECS (true implementation)**
* **Data-Oriented Design (DoD)**
* **Cache-friendly architecture**
* **Full control over the graphics pipeline**

No external frameworks (SDL, SFML, GLFW, etc.).
Rendering is done directly with **OpenGL**, built from scratch.
Only minimal header-only libraries (e.g. `stb_*`, 'cgltf') are used.

---

## Goals

This project serves as:

* A study base for real ECS architecture
* Low-level 2D/3D game development
* Exploration of performance and memory control

---

## Setup

* [Windows](Docs/Setup/Windows.md)
* [Linux](Docs/Setup/Linux.md)

---

## Build Commands

### Build (Host Platform)

Builds the project using the default configuration.

```bash
make
```

### Build and Run

Builds and runs the project.

```bash
make run
```

### Clean

Removes all generated files.

```bash
make clean
```

### Shipping Build

Builds in **Shipping mode** (optimized, no debug features).
Also copies `Config` and `Content` into the output directory, creating a minimal distributable package.

```bash
make ship
```

---

## Notes

Future platform support (e.g. Web):

```bash
make PLATFORM=Web
make run PLATFORM=Web
make ship PLATFORM=Web
```
