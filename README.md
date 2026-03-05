# FPS-Framework-Source

Modular C++ FPS Framework for Unreal Engine 5. Implements Epic Online Services (EOS) for P2P matchmaking, client-side prediction, and a replicated weapon/animation state machine.

**Role:** Systems Engineer / Solo Developer  
**Engine:** Unreal Engine 5.4  
**Language:** C++  

## Overview
A modular First-Person Shooter framework engineered to serve as a scalable foundation for competitive multiplayer games. This project integrates Epic Online Services (EOS) with EIK for peer-to-peer matchmaking, lobby management, and secure user identity management. 

**Note:** This repository contains **source code only**.

---

## Core Systems & Architecture

### 🔫 Advanced Weapon Architecture
* **"True Bore Alignment" (Procedural ADS):** An automated C++ calibration function that utilizes Quaternion math (`FQuat::FindBetweenNormals`) to perfectly align any weapon's barrel vector with the camera's forward vector at runtime, procedurally zeroing any optic without manual offsets.
* **Physics-Based Interaction:** Dropped weapons utilize a `Multicast_DropWeapon` event that applies randomized "Pop and Spin" torque. Players interact with items via an optimized proximity trace that calculates a dot-product "look-score" for precision targeting.
* **Data-Driven Recoil & Configuration:** Weapons utilize a `FWeaponFireConfig` struct to handle fire modes (Single, Burst, Auto) and a modular `URecoilComponent` for data-driven recoil patterns.
* **Optimization:** Weapon impacts utilize a Global Decal Pool capped at 50 instances to strictly preserve rendering budgets and automatically recycle old decals.

### 🏃‍♂️ Movement & Network Replication
* **Client-Side Prediction:** Firing mechanics execute local line traces and Niagara FX immediately for zero-latency feedback. The `Server_FireShot` RPC then validates ammo and recalculates hits using lag compensation logic.
* **Networked Sliding Machine:** Extended the Character Movement Component with a custom `ApplySlidePhysics()` function that dynamically alters ground friction and shrinks the capsule hitbox, replicated securely to all clients.
* **Procedural Arm Interpolation:** Blends raw input sway into the first-person mesh using `FMath::VInterpTo` to create a smooth, responsive combat "glide" during movement and aiming.

### 🖥️ Modular HUD & UI Ecosystem
* **Master HUD Controller:** A decoupled UI architecture utilizing `UModularPlayerHUD` to manage game-mode-specific widgets via `UNamedSlot` containers.
* **Tactical Killfeed:** Engineered with memory safety using `TWeakObjectPtr` and targeted `FTimerHandle` delegates to gracefully manage message decay and prevent memory leaks during high-volume server events.
* **Procedural Crosshair:** Dynamically scales in real-time based on `SpeedSpread` and `FireSpread` calculations fed directly from the character's velocity and trigger state.
* **Networked Minimap Pings:** Utilizes a `Multicast_PingMinimap` RPC to authoritatively broadcast enemy locations (red dots) when unsuppressed weapons are fired.
* **Optimized Compass:** Uses modulo arithmetic to convert 360-degree yaw into precise indices for cardinal directions, avoiding expensive tick-based conditional branches.

### 🎒 Inventory & Economy
* **Universal Slot System:** `UInventoryComponent` supports a `bUniversalWeaponSlots` toggle, allowing server-authoritative modular loadout swapping beyond standard primary/secondary constraints.
* **Dynamic Buy Menu:** Responsive UI that populates weapon columns based on available arrays, featuring real-time affordability checks that automatically disable buttons to prevent invalid server requests.

---

## 🚀 Development Roadmap

**Advanced Weapon Mechanics:**
* Implement procedural weapon sway and wall-collision avoidance.
* Build out the modular Attachment System.
* Integrate Tactical and Lethal equipment.
* Polish weapon animations.

**Locomotion & Player State:**
* Develop a Mantle system.
* Implement networked Spectator states and full-body animation refinements.
* Add configurable health regeneration (Call of Duty style).

**Expanded Game Modes & Logic:**
* Engineer a dedicated C++ Bomb class to finalize Search and Destroy.
* Scale backend to support Team Deathmatch, Free-For-All, and Capture the Flag.

**UI & Social Integration:**
* Complete out-of-match experience (Start Menu, Lobby UI, Friends List, Scoreboard).
* Expand HUD elements (multi-kill tracking, stance indicators, and stacked damage numbers).
* Finalize Audio/VFX implementation and settings menus.
