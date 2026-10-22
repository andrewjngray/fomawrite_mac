// Read-only UI Automation discovery; all interaction enters through Windows SendInput.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Automation;
using System.Threading;
public static class NormalLaunchInput {
    [StructLayout(LayoutKind.Sequential)] struct INPUT { public uint type; public INPUTUNION u; }
    [StructLayout(LayoutKind.Explicit)] struct INPUTUNION {
        [FieldOffset(0)] public MOUSEINPUT mi;
        [FieldOffset(0)] public KEYBDINPUT ki;
    }
    [StructLayout(LayoutKind.Sequential)] struct MOUSEINPUT {
        public int dx, dy; public uint mouseData, dwFlags, time; public UIntPtr extra;
    }
    [StructLayout(LayoutKind.Sequential)] struct KEYBDINPUT {
        public ushort vk, scan; public uint flags, time; public UIntPtr extra;
    }
    [DllImport("user32.dll", SetLastError=true)] static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hwnd, int command);
    [DllImport("user32.dll")] static extern bool SetProcessDPIAware();
    static void Send(INPUT input) {
        if (SendInput(1, new INPUT[] {input}, Marshal.SizeOf(typeof(INPUT))) != 1)
            throw new InvalidOperationException("Windows rejected SendInput: " + Marshal.GetLastWin32Error());
    }
    public static void Initialize() { SetProcessDPIAware(); }
    public static void Foreground(IntPtr hwnd) {
        ShowWindow(hwnd, 9); SetForegroundWindow(hwnd); Thread.Sleep(250);
        if (GetForegroundWindow() != hwnd) throw new InvalidOperationException("Could not foreground acceptance window");
    }
    public static void Key(ushort vk, bool up) {
        INPUT input = new INPUT(); input.type=1; input.u.ki.vk=vk; input.u.ki.flags=up ? 2u : 0u; Send(input);
    }
    public static void Press(ushort vk) { Key(vk, false); Key(vk, true); }
    public static void Chord(ushort modifier, ushort vk) {
        Key(modifier, false); try { Press(vk); } finally { Key(modifier, true); }
    }
    public static void Type(string text) {
        foreach (char ch in text) {
            if (ch == '\n') { Press(0x0D); continue; }
            if (ch == '\r') continue;
            INPUT input = new INPUT(); input.type=1; input.u.ki.scan=ch; input.u.ki.flags=4; Send(input);
            input.u.ki.flags=6; Send(input);
        }
    }
    public static void Click(AutomationElement element) {
        var current=element.Current;
        if (!current.IsEnabled || current.IsOffscreen || current.BoundingRectangle.IsEmpty)
            throw new InvalidOperationException("Cannot click inaccessible control: " + current.Name);
        var rect=current.BoundingRectangle;
        if (!SetCursorPos((int)(rect.Left+rect.Width/2), (int)(rect.Top+rect.Height/2)))
            throw new InvalidOperationException("Cannot position Windows pointer");
        INPUT input=new INPUT(); input.type=0; input.u.mi.dwFlags=2; Send(input);
        input.u.mi.dwFlags=4; Send(input); Thread.Sleep(200);
    }
    public static AutomationElement Root(int pid) {
        return AutomationElement.RootElement.FindFirst(TreeScope.Children,
            new PropertyCondition(AutomationElement.ProcessIdProperty, pid));
    }
    public static AutomationElement ForegroundRoot(int pid) {
        var element=AutomationElement.FromHandle(GetForegroundWindow());
        return element.Current.ProcessId == pid ? element : null;
    }
    public static AutomationElement Find(AutomationElement root, string pattern) {
        if(root == null) return null;
        AutomationElementCollection elements;
        try { elements=root.FindAll(TreeScope.Descendants, Condition.TrueCondition); }
        catch(ElementNotAvailableException) { return null; } // Wait-For re-queries; never count disappearance as success.
        foreach(AutomationElement element in elements) {
            try {
                var item=element.Current;
                if (!item.IsOffscreen && item.IsEnabled && !item.BoundingRectangle.IsEmpty &&
                    System.Text.RegularExpressions.Regex.IsMatch(item.Name.Replace("&", ""), pattern)) return element;
            } catch(ElementNotAvailableException) {}
        }
        return null;
    }
    public static bool Checked(AutomationElement element) {
        object pattern;
        if(element.TryGetCurrentPattern(TogglePattern.Pattern, out pattern))
            return ((TogglePattern)pattern).Current.ToggleState == ToggleState.On;
        if(element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out pattern))
            return ((SelectionItemPattern)pattern).Current.IsSelected;
        throw new InvalidOperationException("Control exposes no checked/selected state: " + element.Current.Name);
    }
    public static string Text(AutomationElement element) {
        object pattern;
        if(element.TryGetCurrentPattern(ValuePattern.Pattern, out pattern)) return ((ValuePattern)pattern).Current.Value;
        if(element.TryGetCurrentPattern(TextPattern.Pattern, out pattern)) return ((TextPattern)pattern).DocumentRange.GetText(-1);
        throw new InvalidOperationException("Editor exposes no readable text pattern");
    }
    public static string[] Describe(AutomationElement root) {
        var result=new List<string>(); if(root == null) return result.ToArray();
        var elements=root.FindAll(TreeScope.Descendants, Condition.TrueCondition);
        foreach(AutomationElement element in elements) try {
            var c=element.Current;
            result.Add(c.ControlType.ProgrammaticName + " | " + c.Name + " | " + c.AutomationId + " | " +
                c.BoundingRectangle + " | offscreen=" + c.IsOffscreen + " | enabled=" + c.IsEnabled);
        } catch(ElementNotAvailableException) {}
        return result.ToArray();
    }
}
