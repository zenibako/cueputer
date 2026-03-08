#pragma once

// ============================================================
//  OSC helpers — pure-logic utilities (no hardware deps)
//  Extracted for testability on native platform.
// ============================================================

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
#include <cstdio>
#include <cstdint>
using String = std::string;
#endif

#include <vector>

namespace OscHelpers {

// ——— Types ——————————————————————————————————————————
enum ArgType : uint8_t { ARG_INT = 0, ARG_FLOAT, ARG_STRING, ARG_BOOL };

struct OscArg {
    ArgType type;
    int     ival;
    float   fval;
    String  sval;
    bool    bval;
};

struct HistoryEntry {
    String  src;
    String  msg;
    bool    sent;   // true = sent, false = received
};

// ——— Constants ———————————————————————————————————————
static const int MAX_HISTORY = 12;

// ——— String truncation ——————————————————————————————
// Truncate s to maxLen chars, appending ".." if truncated.
inline String truncStr(const String& s, int maxLen) {
    if (maxLen <= 0) return "";
#ifdef ARDUINO
    if ((int)s.length() <= maxLen) return s;
    return s.substring(0, maxLen - 2) + "..";
#else
    if ((int)s.length() <= maxLen) return s;
    if (maxLen <= 2) return s.substr(0, maxLen);
    return s.substr(0, maxLen - 2) + "..";
#endif
}

// ——— OSC address normalization ——————————————————————
// Ensure address starts with '/'.
inline String normalizeOscAddress(const String& address) {
    if (address.empty()) return "/";
    if (address[0] != '/') return "/" + address;
    return address;
}

// ——— Format OSC message for display ————————————————
// Returns "address arg1 arg2 ..." string.
inline String formatOscMessage(const String& address,
                               const std::vector<OscArg>& args) {
    String disp = address;
    for (const auto& a : args) {
        disp += " ";
        switch (a.type) {
            case ARG_INT:
#ifdef ARDUINO
                disp += String(a.ival);
#else
                disp += std::to_string(a.ival);
#endif
                break;
            case ARG_FLOAT: {
                char buf[16];
                snprintf(buf, sizeof(buf), "%.2f", a.fval);
                disp += buf;
                break;
            }
            case ARG_STRING:
                disp += a.sval;
                break;
            case ARG_BOOL:
                disp += a.bval ? "T" : "F";
                break;
        }
    }
    return disp;
}

// ——— Format single arg with type prefix ————————————
// Returns "[i] 42", "[f] 3.14", "[s] hello", "[b] true" etc.
inline String formatArgDisplay(const OscArg& arg) {
    switch (arg.type) {
        case ARG_INT:
#ifdef ARDUINO
            return "[i] " + String(arg.ival);
#else
            return "[i] " + std::to_string(arg.ival);
#endif
        case ARG_FLOAT: {
            char buf[24];
            snprintf(buf, sizeof(buf), "[f] %.2f", arg.fval);
            return String(buf);
        }
        case ARG_STRING:
            return "[s] " + arg.sval;
        case ARG_BOOL:
            return arg.bval ? "[b] true" : "[b] false";
    }
    return "";
}

// ——— History buffer append ——————————————————————————
// Appends to history, evicting oldest when full.
inline void histAppend(std::vector<HistoryEntry>& history,
                       const String& label, const String& msg,
                       bool sent, int maxHistory = MAX_HISTORY) {
    if ((int)history.size() >= maxHistory) history.erase(history.begin());
    history.push_back({label, msg, sent});
}

} // namespace OscHelpers
