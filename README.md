# 30-Day Firmware Engineering Deep Dive Challenge

##  Purpose
This repository documents my journey from hobbyist to (hopefully) a more professional firmware engineer. Over 30 days, I'm diving deep into the fundamentals that separate casual embedded projects from production-ready firmware.

## Background
I'm a fresh engineering graduate with hands-on experience in STM32, Arduino, and ESP32 (ESP-IDF). While I've built several projects, I realized I was relying too heavily on libraries and abstraction layers. This challenge is about understanding what happens "under the hood" and learning professional firmware engineering practices.

##  Hardware
- **Primary**: ESP32 Board
- **Secondary**: some LEDs, buttons and sensors

##  Learning Objectives
By the end of this challenge, I aim to understand:
- Memory architecture and management
- Real-time operating systems (RTOS) concepts
- Professional coding standards (MISRA C)
- Bootloaders and firmware updates
- Low-level communication protocols
- Debugging and testing methodologies
- Production-ready firmware practices

## 📅 Challenge Structure

### Week 1: Foundation - Understanding the Metal
**Goal**: Stop abstracting away the hardware. Understand what's really happening.

- **Day 1**: Memory Architecture - ESP32 memory map and bare-metal LED blink
- **Day 2**: Linker Scripts - Custom memory sections and placement
- **Day 3**: Startup Code - Boot sequence from reset vector to main()
- **Day 4**: Memory Management - Stack vs heap, custom allocators
- **Day 5**: Build System - Makefiles, compilation, and linking process
- **Days 6-7**: **Mini Project** - Memory monitor with heap tracking

### Week 2: Real-Time Concepts & Concurrency
**Goal**: Master RTOS fundamentals and safe concurrent programming.

- **Day 8**: Task Scheduling - Preemptive scheduling and priorities
- **Day 9**: Inter-Task Communication - Queues and notifications
- **Day 10**: Synchronization Primitives - Mutexes, semaphores, priority inversion
- **Day 11**: Critical Sections & Interrupts - ISR best practices
- **Day 12**: Race Conditions & Deadlocks - Creating and fixing concurrency bugs
- **Days 13-14**: **Mini Project** - Multi-sensor logger with proper task architecture

### Week 3: Professional Practices
**Goal**: Write code like it's going to production.

- **Day 15**: MISRA C Basics - Static analysis and coding standards
- **Day 16**: Error Handling - Proper error codes and defensive programming
- **Day 17**: Watchdog Timers - Hardware and task watchdogs
- **Day 18**: Low Power Modes - Deep sleep and power management
- **Day 19**: Flash Management - NVS, wear leveling, configuration storage
- **Days 20-21**: **Mini Project** - Production-ready sensor node

### Week 4: Advanced Topics & Integration
**Goal**: Bootloaders, protocols, testing, and debugging.

- **Day 22**: Bootloader Basics - Firmware updates and secure boot
- **Day 23**: Communication Protocols - Manual I2C implementation
- **Day 24**: State Machines - Structured application logic
- **Day 25**: Debugging Techniques - GDB, JTAG, crash analysis
- **Day 26**: Unit Testing - Host-based testing and mocking
- **Days 27-30**: **Final Project** - Complete firmware application

## 📂 Repository Structure
```
firmware-engineering-challenge/
├── README.md (this file)
├── day-01-memory-architecture/
│   ├── main/
│   └── README.md (daily synthesis)
├── day-02-linker-scripts/
│   ├── main/
│   └── README.md
└── ...
```

Each day's folder contains:
- **code/**: All source code and build files
- **notes.md**: Research notes and key learnings
- **README.md**: Synthesis of what I learned that day

## 📖 Resources
- **Books**: "Making Embedded Systems" by Elecia White
- **Documentation**: ESP32 Technical Reference Manual, ESP-IDF docs, FreeRTOS docs
- **Standards**: MISRA C guidelines
- **Tools**: ESP-IDF, GDB, static analyzers (cppcheck)

##  Daily Workflow
1. **Morning (1-2 hours)**: Research and reading
2. **Afternoon (2-3 hours)**: Implementation and hands-on coding
3. **Evening (30-60 mins)**: Documentation and synthesis

## 📊 Progress Tracker

| Day | Topic | Status |
|-----|-------|--------|
| 1 | Memory Architecture | ✅  | 
| 2 | Linker Scripts | ✅  | 
| 3 | Startup Code | ✅  | 
| 4 | Memory Management | ✅  | 
| 5 | Build System | ✅  | 
| 6-7 | Mini Project: Memory Monitor | ✅  | 
| 8 | Task Scheduling | ✅  | 
| 9 | Inter-Task Communication | ✅   |
| 10 | Synchronization Primitives | ✅   |
| 11 | Critical Sections & Interrupts | ✅ | 
| 12 | Race Conditions & Deadlocks | ✅  | 
| 13-14 | Mini Project: Multi-Sensor Logger | ✅  | 
| 15 | MISRA C Basics | ✅ | 
| 16 | Error Handling | ⬜ | 
| 17 | Watchdog Timers | ✅  | 
| 18 | Low Power Modes | ✅ | 
| 19 | Flash Management | ✅ | 
| 20-21 | Mini Project: Production Sensor Node | ✅  | 
| 22 | Bootloader Basics | ✅ | 
| 23 | Communication Protocols | ✅  | 
| 24 | State Machines | ✅ | 
| 25 | Debugging Techniques | ⬜ | 
| 26 | Unit Testing | ✅ | 
| 27-30 | Final Project | ⬜ | 

**Legend**: ⬜ Not Started | 🔄 In Progress | ✅ Completed

##  Success Criteria
- Complete at least 28 of 30 days
- All mini-projects and final project fully documented
- Daily synthesis written for each day
- Code follows professional practices (where applicable)
- Can explain each concept clearly in an interview setting

##  Key Learnings Summary
_This section will be updated throughout the challenge with major insights and "aha!" moments._



##  Notes
- This is a learning journey, not a race. Some days may take longer than others.
- The goal is depth over breadth - truly understanding concepts, not just checking boxes.
- Mistakes and failures are part of the process and will be documented honestly.

##  Connect
If you're on a similar journey or have feedback, feel free to open an issue or reach out!



**Started**: [28th December 2025]  
**Completed**: [Add date when you finish]  
**Total Days**: 27/30