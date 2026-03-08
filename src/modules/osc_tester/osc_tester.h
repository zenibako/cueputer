#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "../../ui/display.h"
#include "../../ui/keyboard.h"
#include "../../ui/menu.h"
#include "../../config/settings.h"

// ============================================================
//  OSC Tester — send/receive OSC over WiFi UDP
//  Implementation: task Z-027
//
//  Features:
//   - Compose and send arbitrary OSC messages (int/float/string/bool)
//   - Listen mode: display incoming OSC in scrollable history
//   - QLab shortcuts: Go / Stop / Panic / Preview
//   - ETC EOS shortcuts: channel check, intensity readout
//   - Network discovery: broadcast query for QLab/EOS endpoints
//   - Message templates: save/load from LittleFS as JSON
//   - Configurable target IP:port (persisted in NVS)
// ============================================================

namespace OscTester {

// ——— Constants ———————————————————————————————————————
static const int    RECV_PORT         = 57121;   // port we listen on
static const int    DEFAULT_SEND_PORT = 8000;    // QLab default
static const int    EOS_PORT          = 8001;    // ETC EOS default
static const int    DISC_PORT         = 9000;    // discovery broadcast port
static const char*  TEMPLATES_FILE    = "/osc_templates.json";
static const int    MAX_TEMPLATES     = 10;
static const int    MAX_HISTORY       = 12;      // display rows in listen mode

// ——— Types ——————————————————————————————————————————
enum ArgType : uint8_t { ARG_INT = 0, ARG_FLOAT, ARG_STRING, ARG_BOOL };

struct OscArg {
    ArgType type;
    int     ival;
    float   fval;
    String  sval;
    bool    bval;
};

struct Template {
    String       name;
    String       address;
    std::vector<OscArg> args;
};

struct HistoryEntry {
    String  src;
    String  msg;
    bool    sent;   // true = sent, false = received
};

// ——— Module state ————————————————————————————————————
static WiFiUDP         _udp;
static String          _targetHost;
static int             _targetPort = DEFAULT_SEND_PORT;
static std::vector<HistoryEntry> _history;
static std::vector<Template>     _templates;
static bool            _fsReady = false;

// ——— Utility: truncate string for display ————————————
static String trunc(const String& s, int maxLen) {
    if ((int)s.length() <= maxLen) return s;
    return s.substring(0, maxLen - 2) + "..";
}

// ——— History append —————————————————————————————————
static void histAppend(const String& label, const String& msg, bool sent) {
    if ((int)_history.size() >= MAX_HISTORY) _history.erase(_history.begin());
    _history.push_back({label, msg, sent});
}

// ——— LittleFS init ——————————————————————————————————
static void ensureFS() {
    if (_fsReady) return;
    if (LittleFS.begin(true)) _fsReady = true;
}

// ——— Load templates from JSON file —————————————————
static void loadTemplates() {
    ensureFS();
    _templates.clear();
    if (!_fsReady || !LittleFS.exists(TEMPLATES_FILE)) return;

    File f = LittleFS.open(TEMPLATES_FILE, "r");
    if (!f) return;

    StaticJsonDocument<4096> doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return; }
    f.close();

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        Template t;
        t.name    = obj["name"].as<String>();
        t.address = obj["address"].as<String>();
        for (JsonObject a : obj["args"].as<JsonArray>()) {
            OscArg arg;
            const char* ty = a["type"] | "i";
            if      (strcmp(ty, "i") == 0) { arg.type = ARG_INT;    arg.ival = a["val"].as<int>(); }
            else if (strcmp(ty, "f") == 0) { arg.type = ARG_FLOAT;  arg.fval = a["val"].as<float>(); }
            else if (strcmp(ty, "s") == 0) { arg.type = ARG_STRING; arg.sval = a["val"].as<String>(); }
            else                           { arg.type = ARG_BOOL;   arg.bval = a["val"].as<bool>(); }
            t.args.push_back(arg);
        }
        _templates.push_back(t);
        if ((int)_templates.size() >= MAX_TEMPLATES) break;
    }
}

// ——— Save templates to JSON file ————————————————————
static void saveTemplates() {
    ensureFS();
    if (!_fsReady) return;

    StaticJsonDocument<4096> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (auto& t : _templates) {
        JsonObject obj = arr.createNestedObject();
        obj["name"]    = t.name;
        obj["address"] = t.address;
        JsonArray args = obj.createNestedArray("args");
        for (auto& a : t.args) {
            JsonObject ao = args.createNestedObject();
            switch (a.type) {
                case ARG_INT:    ao["type"] = "i"; ao["val"] = a.ival; break;
                case ARG_FLOAT:  ao["type"] = "f"; ao["val"] = a.fval; break;
                case ARG_STRING: ao["type"] = "s"; ao["val"] = a.sval; break;
                case ARG_BOOL:   ao["type"] = "b"; ao["val"] = a.bval; break;
            }
        }
    }

    File f = LittleFS.open(TEMPLATES_FILE, "w");
    if (!f) return;
    serializeJson(doc, f);
    f.close();
}

// ——— Build and send an OSC message ——————————————————
static bool sendOsc(const String& host, int port,
                    const String& address, const std::vector<OscArg>& args) {
    if (WiFi.status() != WL_CONNECTED) return false;

    OSCMessage msg(address.c_str());
    for (auto& a : args) {
        switch (a.type) {
            case ARG_INT:    msg.add((int32_t)a.ival); break;
            case ARG_FLOAT:  msg.add(a.fval);          break;
            case ARG_STRING: msg.add(a.sval.c_str());  break;
            case ARG_BOOL:   msg.add(a.bval);          break;
        }
    }

    _udp.beginPacket(host.c_str(), port);
    msg.send(_udp);
    bool ok = _udp.endPacket();

    // Build display string
    String disp = address;
    for (auto& a : args) {
        disp += " ";
        switch (a.type) {
            case ARG_INT:    disp += String(a.ival);   break;
            case ARG_FLOAT:  { char buf[12]; snprintf(buf, 12, "%.2f", a.fval); disp += buf; } break;
            case ARG_STRING: disp += a.sval;           break;
            case ARG_BOOL:   disp += a.bval ? "T" : "F"; break;
        }
    }
    histAppend("TX", disp, true);
    return ok;
}

// ——— Draw scrollable history ————————————————————————
static void drawHistory(int offset) {
    int startY = 18;
    int rowH   = 10;
    int visRows = (Display::H - startY - 12) / rowH;  // ~10 rows

    M5.Display.fillRect(0, startY, Display::W, Display::H - startY - 12, Display::COLOR_BG);
    M5.Display.setTextSize(1);

    for (int i = 0; i < visRows; i++) {
        int idx = offset + i;
        if (idx >= (int)_history.size()) break;
        auto& e = _history[idx];
        int y = startY + i * rowH;
        // Color: sent=cyan, received=green
        uint32_t col = e.sent ? Display::COLOR_ACCENT : Display::COLOR_OK;
        M5.Display.setTextColor(col, Display::COLOR_BG);
        M5.Display.setCursor(2, y);
        String prefix = e.sent ? "> " : "< ";
        String line = prefix + trunc(e.msg, 34);
        M5.Display.print(line);
    }
}

// ——— Argument type picker —————————————————————————
static ArgType pickArgType() {
    const char* types[] = {"int", "float", "string", "bool"};
    int sel = 0;
    auto draw = [&]() {
        Display::clear();
        Display::header("Argument Type");
        for (int i = 0; i < 4; i++) {
            int y = 22 + i * 22;
            bool active = (i == sel);
            M5.Display.fillRect(0, y, Display::W, 20, active ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextColor(active ? Display::COLOR_ACCENT : Display::COLOR_FG,
                                    active ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(16, y + 5);
            M5.Display.print(types[i]);
        }
        Display::status("ENT=pick  i/k=nav  ESC=cancel");
    };
    draw();
    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return ARG_INT;
        if (ev.key == Keyboard::UP && sel > 0)  { sel--; draw(); }
        if (ev.key == Keyboard::DOWN && sel < 3) { sel++; draw(); }
        if (ev.key == Keyboard::ENTER) return (ArgType)sel;
        delay(10);
    }
}

// ——— OSC Message Composer ——————————————————————————
static void composeAndSend() {
    if (_targetHost.isEmpty()) {
        Display::clear();
        Display::header("OSC Composer");
        Display::centerText("No target host set!", 50, 1, Display::COLOR_WARN);
        Display::centerText("Use Set Target first", 65, 1, Display::COLOR_MUTED);
        Display::status("ESC=back");
        delay(2000);
        return;
    }

    // Step 1: address
    Display::clear();
    Display::header("OSC Composer");
    M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(4, 22);
    M5.Display.print("Address (e.g. /cue/1/go):");
    String address = Keyboard::readLine(4, 36, 48);
    if (address.isEmpty()) return;
    if (address[0] != '/') address = "/" + address;

    // Step 2: add arguments (optional, repeat)
    std::vector<OscArg> args;
    while (true) {
        Display::clear();
        Display::header("OSC Composer");

        M5.Display.setTextColor(Display::COLOR_ACCENT, Display::COLOR_BG);
        M5.Display.setTextSize(1);
        M5.Display.setCursor(4, 20);
        M5.Display.print(trunc(address, 34));

        // List current args
        int y = 34;
        for (int i = 0; i < (int)args.size(); i++) {
            M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);
            M5.Display.setCursor(4, y);
            String s;
            switch (args[i].type) {
                case ARG_INT:    s = "[i] " + String(args[i].ival);   break;
                case ARG_FLOAT:  { char b[16]; snprintf(b,16,"[f] %.2f",args[i].fval); s=b; } break;
                case ARG_STRING: s = "[s] " + args[i].sval;           break;
                case ARG_BOOL:   s = "[b] " + String(args[i].bval ? "true" : "false"); break;
            }
            M5.Display.print(trunc(s, 34));
            y += 12;
        }

        Display::status("ENT=add arg  S=send  ESC=cancel");

        Keyboard::Event ev = Keyboard::waitKey();
        if (ev.key == Keyboard::BACK) return;

        if (ev.key == Keyboard::CHAR && (ev.ch == 's' || ev.ch == 'S')) {
            // Send now
            bool ok = sendOsc(_targetHost, _targetPort, address, args);
            Display::status(ok ? "Sent!" : "Send failed", ok ? Display::COLOR_OK : Display::COLOR_ERROR);
            delay(1200);
            return;
        }

        if (ev.key == Keyboard::ENTER) {
            // Pick type, then enter value
            ArgType ty = pickArgType();
            OscArg arg;
            arg.type = ty;

            Display::clear();
            Display::header("OSC Composer");
            M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(4, 22);

            switch (ty) {
                case ARG_INT: {
                    M5.Display.print("Integer value:");
                    String v = Keyboard::readLine(4, 36, 12);
                    arg.ival = v.toInt();
                    break;
                }
                case ARG_FLOAT: {
                    M5.Display.print("Float value:");
                    String v = Keyboard::readLine(4, 36, 16);
                    arg.fval = v.toFloat();
                    break;
                }
                case ARG_STRING: {
                    M5.Display.print("String value:");
                    arg.sval = Keyboard::readLine(4, 36, 32);
                    break;
                }
                case ARG_BOOL: {
                    M5.Display.print("Bool (T=true / F=false):");
                    Keyboard::Event bev = Keyboard::waitKey();
                    arg.bval = (bev.key == Keyboard::CHAR &&
                                (bev.ch == 'T' || bev.ch == 't' || bev.ch == '1'));
                    break;
                }
            }
            args.push_back(arg);
        }
    }
}

// ——— Listen Mode ————————————————————————————————————
static void listenMode() {
    Display::clear();
    Display::header("OSC Listen");

    if (WiFi.status() != WL_CONNECTED) {
        Display::centerText("WiFi not connected", 55, 1, Display::COLOR_WARN);
        Display::status("ESC=back");
        delay(2000);
        return;
    }

    _udp.begin(RECV_PORT);

    char portStr[32];
    snprintf(portStr, sizeof(portStr), "Listening on :%d  ESC=back", RECV_PORT);
    Display::status(portStr, Display::COLOR_OK);

    int histOffset = 0;
    bool dirty = true;

    auto redraw = [&]() {
        Display::header("OSC Listen ← RX");
        drawHistory(histOffset);
        snprintf(portStr, sizeof(portStr), "Listening :%d  i/k=scroll  ESC=back", RECV_PORT);
        Display::status(portStr, Display::COLOR_OK);
    };

    uint32_t lastPoll = 0;
    while (true) {
        // Non-blocking keyboard check
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) break;
        if (ev.key == Keyboard::UP && histOffset > 0) { histOffset--; dirty = true; }
        if (ev.key == Keyboard::DOWN) {
            int maxOff = (int)_history.size() > 1 ? (int)_history.size() - 1 : 0;
            if (histOffset < maxOff) { histOffset++; dirty = true; }
        }

        // Poll UDP every 50ms
        if (millis() - lastPoll > 50) {
            lastPoll = millis();
            int sz = _udp.parsePacket();
            if (sz > 0) {
                char buf[256];
                int len = _udp.read(buf, sizeof(buf) - 1);
                buf[len] = '\0';

                // Parse the OSC message
                OSCMessage msg;
                msg.fill((uint8_t*)buf, len);

                if (!msg.hasError()) {
                    char addr[64] = "";
                    msg.getAddress(addr, 0, sizeof(addr));

                    String src = _udp.remoteIP().toString() + ":" + String(_udp.remotePort());
                    String txt = String(addr);

                    // Append args summary
                    for (int i = 0; i < msg.size(); i++) {
                        txt += " ";
                        if      (msg.isInt(i))    txt += String(msg.getInt(i));
                        else if (msg.isFloat(i))  { char fb[12]; snprintf(fb,12,"%.2f",msg.getFloat(i)); txt += fb; }
                        else if (msg.isString(i)) { char sb[32]; msg.getString(i, sb, 32); txt += sb; }
                        else if (msg.isBoolean(i)) txt += (msg.getBoolean(i) ? "T" : "F");
                    }
                    histAppend(src, txt, false);

                    // Auto-scroll to bottom
                    histOffset = _history.size() > 0 ? (int)_history.size() - 1 : 0;
                    dirty = true;
                }
            }
        }

        if (dirty) {
            redraw();
            dirty = false;
        }

        delay(10);
    }

    _udp.stop();
}

// ——— QLab Shortcuts ————————————————————————————————
static void qlabShortcuts() {
    if (_targetHost.isEmpty()) {
        Display::clear();
        Display::header("QLab Shortcuts");
        Display::centerText("No target host set!", 55, 1, Display::COLOR_WARN);
        Display::status("ESC=back");
        delay(2000);
        return;
    }

    struct Shortcut { const char* label; const char* addr; const char* key; char hotkey; };
    static const Shortcut shortcuts[] = {
        { "Go",      "/cue/selected/start",        "G", 'g' },
        { "Stop",    "/cue/selected/stop",         "S", 's' },
        { "Pause",   "/cue/selected/pause",        "P", 'p' },
        { "Panic",   "/panic",                     "!", '!' },
        { "Preview", "/cue/selected/preview",      "V", 'v' },
        { "Rewind",  "/cue/selected/togglePaused", "R", 'r' },
    };
    const int N = 6;

    auto draw = [&](int sel) {
        Display::clear();
        Display::header("QLab Shortcuts");
        for (int i = 0; i < N; i++) {
            int y = 20 + i * 17;
            bool active = (i == sel);
            M5.Display.fillRect(0, y, Display::W, 16, active ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextColor(active ? Display::COLOR_ACCENT : Display::COLOR_FG,
                                    active ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(6, y + 3);
            M5.Display.print(shortcuts[i].label);
            M5.Display.setTextColor(Display::COLOR_MUTED, active ? 0x1A3A5C : Display::COLOR_BG);
            int kw = strlen(shortcuts[i].key) * 6;
            M5.Display.setCursor(Display::W - kw - 6, y + 3);
            M5.Display.print(shortcuts[i].key);
        }
        char tgt[48];
        snprintf(tgt, sizeof(tgt), "%s:%d  ENT=fire  ESC=back",
                 trunc(_targetHost, 12).c_str(), _targetPort);
        Display::status(tgt, Display::COLOR_MUTED);
    };

    int sel = 0;
    draw(sel);

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        if (ev.key == Keyboard::UP   && sel > 0)   { sel--; draw(sel); }
        if (ev.key == Keyboard::DOWN && sel < N-1) { sel++; draw(sel); }

        if (ev.key == Keyboard::ENTER || ev.key == Keyboard::CHAR) {
            int fire = sel;
            if (ev.key == Keyboard::CHAR) {
                // Match hotkey (case-insensitive)
                char ch = tolower(ev.ch);
                int found = -1;
                for (int i = 0; i < N; i++) {
                    if (tolower(shortcuts[i].hotkey) == ch) { found = i; break; }
                }
                if (found < 0) { delay(10); continue; }
                fire = found;
            }
            std::vector<OscArg> empty;
            bool ok = sendOsc(_targetHost, _targetPort, shortcuts[fire].addr, empty);
            Display::status(ok ? (String(shortcuts[fire].label) + " sent!").c_str()
                               : "Send failed",
                            ok ? Display::COLOR_OK : Display::COLOR_ERROR);
            delay(800);
            draw(sel);
        }

        delay(10);
    }
}

// ——— ETC EOS Shortcuts —————————————————————————————
static void eosShortcuts() {
    if (_targetHost.isEmpty()) {
        Display::clear();
        Display::header("EOS Shortcuts");
        Display::centerText("No target host set!", 55, 1, Display::COLOR_WARN);
        Display::status("ESC=back");
        delay(2000);
        return;
    }

    auto sendEos = [&](const String& addr, const std::vector<OscArg>& args) {
        sendOsc(_targetHost, EOS_PORT, addr, args);
    };

    auto draw = [&](int sel) {
        Display::clear();
        Display::header("ETC EOS");

        const char* items[] = {
            "Chan Check (type ch#)",
            "Intensity (ch# + lvl)",
            "Go (next cue)",
            "Stop / Back",
        };
        const int N = 4;
        for (int i = 0; i < N; i++) {
            int y = 22 + i * 22;
            bool active = (i == sel);
            M5.Display.fillRect(0, y, Display::W, 20, active ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextColor(active ? Display::COLOR_ACCENT : Display::COLOR_FG,
                                    active ? 0x1A3A5C : Display::COLOR_BG);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(6, y + 5);
            M5.Display.print(items[i]);
        }
        char tgt[48];
        snprintf(tgt, sizeof(tgt), "%s:%d  ENT=select  ESC=back",
                 trunc(_targetHost, 10).c_str(), EOS_PORT);
        Display::status(tgt, Display::COLOR_MUTED);
    };

    int sel = 0;
    draw(sel);

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        if (ev.key == Keyboard::UP   && sel > 0) { sel--; draw(sel); }
        if (ev.key == Keyboard::DOWN && sel < 3) { sel++; draw(sel); }

        if (ev.key == Keyboard::ENTER) {
            switch (sel) {
                case 0: { // Channel check
                    Display::clear(); Display::header("EOS Chan Check");
                    M5.Display.setCursor(4, 24); M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
                    M5.Display.print("Channel #:");
                    String ch = Keyboard::readLine(4, 38, 6);
                    if (!ch.isEmpty()) {
                        std::vector<OscArg> args = {{ARG_INT, ch.toInt()}};
                        sendEos("/eos/chan/" + ch, args);
                        Display::status(("Chan " + ch + " selected").c_str(), Display::COLOR_OK);
                        delay(1000);
                    }
                    break;
                }
                case 1: { // Set intensity
                    Display::clear(); Display::header("EOS Intensity");
                    M5.Display.setCursor(4, 24); M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
                    M5.Display.print("Channel #:");
                    String ch = Keyboard::readLine(4, 38, 6);
                    M5.Display.setCursor(4, 56); M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
                    M5.Display.print("Level (0-100):");
                    String lvl = Keyboard::readLine(4, 70, 4);
                    if (!ch.isEmpty() && !lvl.isEmpty()) {
                        std::vector<OscArg> args = {{ARG_INT, lvl.toInt()}};
                        sendEos("/eos/chan/" + ch + "/at/" + lvl, args);
                        Display::status(("Ch" + ch + " @ " + lvl + "%").c_str(), Display::COLOR_OK);
                        delay(1000);
                    }
                    break;
                }
                case 2: { // Go
                    std::vector<OscArg> empty;
                    sendEos("/eos/key/go_0", empty);
                    Display::status("EOS GO sent", Display::COLOR_OK);
                    delay(800);
                    break;
                }
                case 3: { // Stop/Back
                    std::vector<OscArg> empty;
                    sendEos("/eos/key/stop", empty);
                    Display::status("EOS STOP sent", Display::COLOR_WARN);
                    delay(800);
                    break;
                }
            }
            draw(sel);
        }

        delay(10);
    }
}

// ——— Network Discovery ——————————————————————————————
static void networkDiscover() {
    Display::clear();
    Display::header("OSC Discovery");

    if (WiFi.status() != WL_CONNECTED) {
        Display::centerText("WiFi not connected", 55, 1, Display::COLOR_WARN);
        Display::status("ESC=back");
        delay(2000);
        return;
    }

    Display::centerText("Broadcasting query...", 40, 1, Display::COLOR_MUTED);
    Display::centerText("Listening 3 seconds", 55, 1, Display::COLOR_MUTED);

    // Send QLab workspace discovery broadcast
    WiFiUDP disc;
    disc.begin(DISC_PORT);
    IPAddress broadcast = WiFi.localIP();
    broadcast[3] = 255;

    {
        OSCMessage q("/qlab/workspaces");
        disc.beginPacket(broadcast, 53000);
        q.send(disc);
        disc.endPacket();
    }
    {
        OSCMessage q("/eos/ping");
        disc.beginPacket(broadcast, EOS_PORT);
        q.send(disc);
        disc.endPacket();
    }

    std::vector<String> found;
    uint32_t deadline = millis() + 3000;

    while (millis() < deadline) {
        int sz = disc.parsePacket();
        if (sz > 0) {
            char buf[256];
            int len = disc.read(buf, sizeof(buf) - 1);
            buf[len] = '\0';
            OSCMessage msg;
            msg.fill((uint8_t*)buf, len);
            if (!msg.hasError()) {
                String src = disc.remoteIP().toString() + ":" + String(disc.remotePort());
                if (std::find(found.begin(), found.end(), src) == found.end())
                    found.push_back(src);
            }
        }
        delay(50);
    }
    disc.stop();

    Display::clear();
    Display::header("OSC Discovery");

    if (found.empty()) {
        Display::centerText("No endpoints found", 50, 1, Display::COLOR_WARN);
        Display::centerText("Try manual IP entry", 65, 1, Display::COLOR_MUTED);
    } else {
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(Display::COLOR_OK, Display::COLOR_BG);
        M5.Display.setCursor(4, 22);
        M5.Display.print("Found:");
        int y = 36;
        for (auto& f : found) {
            M5.Display.setTextColor(Display::COLOR_FG, Display::COLOR_BG);
            M5.Display.setCursor(4, y);
            M5.Display.print(trunc(f, 34));
            y += 14;
        }
    }

    Display::status("ESC=back");
    while (Keyboard::waitKey().key != Keyboard::BACK) {}
}

// ——— Templates UI ——————————————————————————————————
static void templatesMenu() {
    loadTemplates();

    auto draw = [&](int sel) {
        Display::clear();
        Display::header("OSC Templates");
        if (_templates.empty()) {
            Display::centerText("No templates saved", 50, 1, Display::COLOR_MUTED);
            Display::centerText("N=new template", 65, 1, Display::COLOR_MUTED);
        } else {
            const int VIS = 5;
            int offset = sel >= VIS ? sel - VIS + 1 : 0;
            for (int i = 0; i < VIS && (offset+i) < (int)_templates.size(); i++) {
                int idx = offset + i;
                int y = 20 + i * 19;
                bool active = (idx == sel);
                M5.Display.fillRect(0, y, Display::W, 18, active ? 0x1A3A5C : Display::COLOR_BG);
                M5.Display.setTextColor(active ? Display::COLOR_ACCENT : Display::COLOR_FG,
                                        active ? 0x1A3A5C : Display::COLOR_BG);
                M5.Display.setTextSize(1);
                M5.Display.setCursor(4, y + 4);
                M5.Display.print(trunc(_templates[idx].name, 16));
                M5.Display.setTextColor(Display::COLOR_MUTED, active ? 0x1A3A5C : Display::COLOR_BG);
                M5.Display.setCursor(105, y + 4);
                M5.Display.print(trunc(_templates[idx].address, 16));
            }
        }
        Display::status("ENT=send  N=new  D=del  ESC=back");
    };

    int sel = 0;
    draw(sel);

    while (true) {
        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        if (ev.key == Keyboard::UP && sel > 0)   { sel--; draw(sel); }
        if (ev.key == Keyboard::DOWN && sel < (int)_templates.size() - 1) { sel++; draw(sel); }

        if (ev.key == Keyboard::ENTER && !_templates.empty()) {
            // Send selected template
            auto& t = _templates[sel];
            bool ok = sendOsc(_targetHost, _targetPort, t.address, t.args);
            Display::status(ok ? ("Sent: " + t.name).c_str() : "Send failed",
                            ok ? Display::COLOR_OK : Display::COLOR_ERROR);
            delay(1000);
            draw(sel);
        }

        if (ev.key == Keyboard::CHAR && (ev.ch == 'n' || ev.ch == 'N')) {
            // New template — first compose a message
            Display::clear();
            Display::header("New Template");
            M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(4, 22);
            M5.Display.print("Template name:");
            String name = Keyboard::readLine(4, 36, 20);
            if (name.isEmpty()) { draw(sel); continue; }

            M5.Display.setCursor(4, 54);
            M5.Display.print("OSC address:");
            String addr = Keyboard::readLine(4, 68, 40);
            if (addr.isEmpty()) { draw(sel); continue; }
            if (addr[0] != '/') addr = "/" + addr;

            Template t;
            t.name    = name;
            t.address = addr;
            // Keep it simple: no arg entry in template (use compose for that)
            if ((int)_templates.size() < MAX_TEMPLATES) {
                _templates.push_back(t);
                saveTemplates();
                sel = _templates.size() - 1;
            }
            draw(sel);
        }

        if (ev.key == Keyboard::CHAR && (ev.ch == 'd' || ev.ch == 'D') &&
            !_templates.empty()) {
            _templates.erase(_templates.begin() + sel);
            saveTemplates();
            if (sel > 0) sel--;
            draw(sel);
        }

        delay(10);
    }
}

// ——— Set Target (host + port) ————————————————————
static void setTarget() {
    Settings::Config cfg = Settings::load();
    _targetHost = cfg.oscHost;
    _targetPort = cfg.oscPort;

    Display::clear();
    Display::header("Set OSC Target");

    M5.Display.setTextColor(Display::COLOR_MUTED, Display::COLOR_BG);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(4, 22);
    M5.Display.print("Target IP:");
    String host = Keyboard::readLine(4, 36, 20);

    M5.Display.setCursor(4, 54);
    M5.Display.print("Port (default 8000):");
    String portStr = Keyboard::readLine(4, 68, 6);

    if (!host.isEmpty()) {
        _targetHost = host;
        cfg.oscHost = host;
    }
    if (!portStr.isEmpty()) {
        _targetPort = portStr.toInt();
        cfg.oscPort = _targetPort;
    }
    Settings::save(cfg);

    char buf[48];
    snprintf(buf, sizeof(buf), "Target: %s:%d", _targetHost.c_str(), _targetPort);
    Display::status(buf, Display::COLOR_OK);
    delay(1500);
}

// ——— Message History viewer —————————————————————————
static void viewHistory() {
    int offset = 0;
    bool dirty = true;

    while (true) {
        if (dirty) {
            Display::clear();
            Display::header("Message History");
            if (_history.empty()) {
                Display::centerText("No messages yet", 55, 1, Display::COLOR_MUTED);
            } else {
                drawHistory(offset);
            }
            Display::status("i/k=scroll  ESC=back");
            dirty = false;
        }

        Keyboard::Event ev = Keyboard::poll();
        if (ev.key == Keyboard::BACK) return;
        if (ev.key == Keyboard::UP && offset > 0) { offset--; dirty = true; }
        if (ev.key == Keyboard::DOWN) {
            if ((int)_history.size() > 1 && offset < (int)_history.size() - 1) {
                offset++; dirty = true;
            }
        }
        delay(10);
    }
}

// ——— Top-level run() ————————————————————————————————
inline void run() {
    // Load config
    Settings::Config cfg = Settings::load();
    _targetHost = cfg.oscHost;
    _targetPort = cfg.oscPort > 0 ? cfg.oscPort : DEFAULT_SEND_PORT;

    Menu::MenuScreen menu("OSC Tester");

    menu.addItem("Send Message",  "compose & send", []() { composeAndSend(); });
    menu.addItem("Listen Mode",   "rx incoming",    []() { listenMode(); });
    menu.addItem("QLab",          "shortcuts",      []() { qlabShortcuts(); });
    menu.addItem("ETC EOS",       "shortcuts",      []() { eosShortcuts(); });
    menu.addItem("Discover",      "find endpoints", []() { networkDiscover(); });
    menu.addItem("Templates",     "saved msgs",     []() { templatesMenu(); });
    menu.addItem("Set Target",    "IP:port",        []() { setTarget(); });
    menu.addItem("History",       "msg log",        []() { viewHistory(); });

    menu.run();
}

} // namespace OscTester
