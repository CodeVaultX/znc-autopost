/*
 * Copyright (C) 2025 ZNC Autopost Module
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <znc/znc.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>
#include <vector>
#include <ctime>

class CAutopostedMessage {
public:
    CString target;      // #channel or nick
    CString message;
    CString days;        // Mon,Tue,... or "once"
    int hour;
    int minute;
    time_t lastSent = 0;
    bool once = false;
    bool isPM = false;   // automatically set based on target

    CString Serialize() const {
        return CString(target) + "|" +
               CString(hour) + "|" +
               CString(minute) + "|" +
               CString(days) + "|" +
               CString(message) + "|" +
               CString(isPM ? "1" : "0");
    }

    static CAutopostedMessage Parse(const CString& line) {
        CAutopostedMessage s;
        VCString parts;
        line.Split("|", parts);
        if (parts.size() >= 6) {
            s.target = parts[0];
            s.hour = parts[1].ToInt();
            s.minute = parts[2].ToInt();
            s.days = parts[3];
            s.message = parts[4];
            s.isPM = (parts[5] == "1");
            s.once = s.days.Equals("once");
        }
        return s;
    }
};

class CAutopostMod : public CModule {
    std::vector<CAutopostedMessage> tasks;
    const VCString dayNames = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

public:
    MODCONSTRUCTOR(CAutopostMod) {}

    bool OnLoad(const CString& sArgs, CString& sMessage) override {
        LoadTasks();
        SetTimer();
        return true;
    }

    void LoadTasks() {
        tasks.clear();
        CString raw = GetNV("tasks");
        if (!raw.empty()) {
            VCString lines;
            raw.Split("\n", lines);
            for (const CString& l : lines) {
                if (!l.Trim_n().empty())
                    tasks.push_back(CAutopostedMessage::Parse(l));
            }
        }
    }

    void SaveTasks() {
        CString out;
        for (const auto& t : tasks)
            out += t.Serialize() + "\n";
        SetNV("tasks", out);
    }

    class CAutopostTimer : public CTimer {
    public:
        CAutopostMod* mod;
        CAutopostTimer(CAutopostMod* m)
            : CTimer(m, 60, 0, "autopost-timer", "") { mod = m; }
        void RunJob() override { mod->CheckTasks(); }
    };

    void SetTimer() { AddTimer(new CAutopostTimer(this)); }

    void CheckTasks() {
        time_t now = time(nullptr);
        struct tm* local = localtime(&now);  // server time

        int currentH = local->tm_hour;
        int currentM = local->tm_min;
        int wd = local->tm_wday;  // 0=Sun

        if (wd == 0) wd = 6; else wd -= 1;  // map Sun->6, Mon->0

        std::vector<int> toDelete;

        for (size_t i=0; i<tasks.size(); i++) {
            auto& t = tasks[i];

            if (t.hour != currentH || t.minute != currentM) continue;
            if (t.lastSent != 0 && now - t.lastSent < 60) continue;

            if (t.once) {
                SendMessage(t.target, t.message, t.isPM);
                toDelete.push_back(i);
                continue;
            }

            VCString allowedDays;
            t.days.Split(",", allowedDays);
            if (!allowedDays.empty() && !allowedDays[0].Equals("once")) {
                bool ok = false;
                for (const CString& d : allowedDays) {
                    if (d.Equals(dayNames[wd])) ok = true;
                }
                if (!ok) continue;
            }

            SendMessage(t.target, t.message, t.isPM);
            t.lastSent = now;
        }

        for (int i=(int)toDelete.size()-1; i>=0; i--)
            tasks.erase(tasks.begin() + toDelete[i]);

        if (!toDelete.empty())
            SaveTasks();
    }

    void SendMessage(const CString& target, const CString& msg, bool pm) {
        if (!GetNetwork()) return;
        GetNetwork()->PutIRC("PRIVMSG " + target + " :" + msg);
    }

    void OnModCommand(const CString& cmd) override {
        CString c = cmd.Token(0).AsLower();

        if (c == "help") {
            PutModule("Autopost Module Commands:");
            PutModule("add <target> <HH:MM> <days|once> <message>  - Add one-time or specific days task");
            PutModule("add_daily <target> <HH:MM> <message>       - Add a daily task");
            PutModule("del <index>                                  - Delete a task by index");
            PutModule("list                                         - List all tasks");
            PutModule("Examples:");
            PutModule("add #Online 15:30 once Hello guys!");
            PutModule("add Nick 15:30 once Hi there!");
            PutModule("add_daily #NBA 13:00 I love basketball!");
            PutModule("add_daily Nick 13:00 just testing :)");
            return;
        }

        else if (c == "add") {
            CString target = cmd.Token(1);
            CString time = cmd.Token(2);
            CString days = cmd.Token(3);
            CString message = cmd.Token(4, true);

            if (target.empty() || time.empty() || days.empty() || message.empty()) {
                PutModule("Usage: add <target> <HH:MM> <days|once> <message>");
                return;
            }

            int hour=0, minute=0;
            VCString parts; time.Split(":", parts);
            if (parts.size() == 2) { hour = parts[0].ToInt(); minute = parts[1].ToInt(); }

            CAutopostedMessage t;
            t.target = target;
            t.hour = hour;
            t.minute = minute;
            t.days = days;
            t.once = days.Equals("once");
            t.message = message;
            t.isPM = !target.StartsWith("#");

            tasks.push_back(t);
            SaveTasks();
            PutModule("Task added.");
        }

        else if (c == "add_daily") {
            CString target = cmd.Token(1);
            CString time = cmd.Token(2);
            CString message = cmd.Token(3, true);

            if (target.empty() || time.empty() || message.empty()) {
                PutModule("Usage: add_daily <target> <HH:MM> <message>");
                return;
            }

            int hour=0, minute=0;
            VCString parts; time.Split(":", parts);
            if (parts.size() == 2) { hour = parts[0].ToInt(); minute = parts[1].ToInt(); }

            CAutopostedMessage t;
            t.target = target;
            t.hour = hour;
            t.minute = minute;
            t.days = "Mon,Tue,Wed,Thu,Fri,Sat,Sun";
            t.once = false;
            t.message = message;
            t.isPM = !target.StartsWith("#");

            tasks.push_back(t);
            SaveTasks();
            PutModule("Daily task added.");
        }

        else if (c == "del") {
            int idx = cmd.Token(1).ToInt();
            if (idx < 1 || idx > (int)tasks.size()) { PutModule("Invalid index."); return; }
            tasks.erase(tasks.begin() + (idx-1));
            SaveTasks();
            PutModule("Deleted.");
        }

        else if (c == "list") {
            if (tasks.empty()) { PutModule("No tasks."); return; }
            int i=1;
            for (auto& t : tasks) {
                char buf[6]; snprintf(buf,sizeof(buf),"%02d:%02d",t.hour,t.minute);
                CString info = CString(i) + ") " + t.target + " - " + CString(buf) + " - " + t.days + " - " + t.message;
                if (t.lastSent) info += " (last: " + CString(t.lastSent) + ")";
                if (t.isPM) info += " [PM]";
                PutModule(info);
                i++;
            }
        }

        else {
            PutModule("Type /msg *autopost help for command list and examples.");
        }
    }
};

MODULEDEFS(CAutopostMod, "Autopost messages module with channel and PM support, automatic target detection");
