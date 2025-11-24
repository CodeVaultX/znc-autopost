# ZNC Autopost Module
**Autopost Module for ZNC**  
Send scheduled messages to channels or PMs. Supports one-time, specific days, and daily tasks.

---

## Features

- Send **one-time** messages (`once`)  
- Send messages on **specific weekdays** (`Mon,Tue,...`)  
- Send **daily messages**  
- Automatically detects **PM vs channel**:
  - `#Channel` → channel message  
  - `Nick` → private message  
- Server time is used (UTC by default)  
- List tasks and see last sent timestamp  
- Delete tasks by index  

---

## Installation

1. Place `autopost.cpp` in your ZNC modules directory:

```bash
~/.znc/modules/autopost.cpp
```


---

### Option 2: Build from source

If you prefer to compile it manually:

  ```bash
  znc-buildmod autopost.cpp
  ```

This will generate `autopost.so` in your current directory. Move it to your ZNC module directory and reload ZNC.

---
## 🚀 Usage
- You must load the module from ZNC first:
  ```
  /znc LoadMod autopost
   ```
  or
  ```
  /msg *status LoadMod autopost
  ```

You can control the module via commands: 
   ```
   /msg *autopost help
   ```
- Shows all commands and usage examples.
---

### Commands (Add a task)
```
/msg *autopost add <target> <HH:MM> <days|once> <message>
```
	. target → #channel or nick
	. HH:MM → 24-hour server time
	. days → Comma separated weekdays (Mon,Tue,...) or once
	. message → Text to send


## 💬 Example

```
/msg *autopost add #NBA 15:30 once Game starts!
/msg *autopost add Nick 15:30 once Hi there!
/msg *autopost add #NBA 13:00 Mon,Wed,Fri Weekly reminder!
```
---

### Commands (Add a daily task)
```
/msg *autopost add_daily <target> <HH:MM> <message>
```

## 💬 Example

```
/msg *autopost add_daily #NBA 13:00 I love basketball!
/msg *autopost add_daily Nick 13:00 just testing :)
```

---

### Commands (List tasks)
```
/msg *autopost list
```
	- Shows all scheduled tasks with:
		. Target
		. Time
		. Days
		. Message
		. Last sent timestamp
		. PM indicator if applicable

## 💬 Example

```
/msg *autopost list
```
Example output:
```
1) #NBA - 15:22 - Mon,Tue,Wed,Thu,Fri,Sat,Sun - Daily update! (last: 1763986952)
2) Nick - 15:30 - once - Hi there! [PM]
```
---

### Commands (Delete a task)
```
/msg *autopost del <index>
```

## 💬 Example

```
/msg *autopost del 1
```
---

## 💬 Example Workflow

1) Add a one-time channel message at 15:30:
```
/msg *autopost add #NBA 15:30 once Game starts!
```
2) Add a weekly PM reminder for Mon, Wed, Fri at 13:00:
```
/msg *autopost add Nick 13:00 Mon,Wed,Fri Hi there!
```
3) Add a daily message to a channel at 13:00:
```
/msg *autopost add_daily #NBA 13:00 Daily update!
```
4) List all tasks:
```
/msg *autopost list
```
5) Delete a task:
```
/msg *autopost del 1
```
6) Get help anytime:
```
/msg *autopost help
```
---

# Notes
```
Time is server time, no timezone conversion.

Daily tasks repeat indefinitely until deleted.

One-time tasks are automatically removed after sending.

PM vs channel is detected automatically; no [pm] parameter needed.

days should be capitalized English day abbreviations (Mon,Tue,Wed,Thu,Fri,Sat,Sun).
```
---

## 📃 License
```
Licensed under the [Apache License 2.0](https://github.com/CodeVaultX/znc-autopost/blob/main/LICENSE).
```
---
