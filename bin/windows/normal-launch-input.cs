// Read-only UI Automation discovery; all interaction enters through Windows SendInput.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Automation;
using System.Threading;
using System.Diagnostics;
using System.Text.RegularExpressions;
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
    [StructLayout(LayoutKind.Sequential)] struct POINT { public int x, y; }
    [DllImport("user32.dll", SetLastError=true)] static extern bool SetPhysicalCursorPos(int x, int y);
    [DllImport("user32.dll", SetLastError=true)] static extern bool GetPhysicalCursorPos(out POINT point);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern bool IsWindow(IntPtr hwnd);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hwnd, int command);
    [DllImport("user32.dll", SetLastError=true)] static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] static extern IntPtr GetThreadDpiAwarenessContext();
    [DllImport("user32.dll")] static extern bool AreDpiAwarenessContextsEqual(IntPtr first, IntPtr second);
    static void Send(INPUT input) {
        if (SendInput(1, new INPUT[] {input}, Marshal.SizeOf(typeof(INPUT))) != 1)
            throw new InvalidOperationException("Windows rejected SendInput: " + Marshal.GetLastWin32Error());
    }
    public static void Initialize() {
        var physicalContext=new IntPtr(-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        if(SetThreadDpiAwarenessContext(physicalContext)==IntPtr.Zero ||
            !AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(), physicalContext))
            throw new InvalidOperationException("Cannot establish physical per-monitor-v2 DPI context: " + Marshal.GetLastWin32Error());
    }
    public static void MovePhysical(int x, int y) {
        if(!SetPhysicalCursorPos(x, y))
            throw new InvalidOperationException("Cannot position physical Windows pointer: " + Marshal.GetLastWin32Error());
        POINT observed;
        if(!GetPhysicalCursorPos(out observed) || observed.x!=x || observed.y!=y)
            throw new InvalidOperationException("Physical pointer position did not match the requested UIA coordinates");
    }
    static IntPtr expectedInputWindow;
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    static uint expectedProcessId;
    [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] static extern bool AttachThreadInput(uint source, uint target, bool attach);
    public static void Foreground(IntPtr hwnd) {
        ShowWindow(hwnd, 9); SetForegroundWindow(hwnd); Thread.Sleep(100);
        if(GetForegroundWindow()!=hwnd) {
            uint ignored;
            uint foregroundThread=GetWindowThreadProcessId(GetForegroundWindow(), out ignored);
            uint callerThread=GetCurrentThreadId();
            bool attached=false;
            try {
                if(foregroundThread!=0 && foregroundThread!=callerThread)
                    attached=AttachThreadInput(callerThread, foregroundThread, true);
                SetForegroundWindow(hwnd);
            } finally {
                if(attached) AttachThreadInput(callerThread, foregroundThread, false);
            }
            Thread.Sleep(150);
        }
        for(int attempt=0; attempt<2 && GetForegroundWindow()!=hwnd; attempt++) {
            Press(0x12); SetForegroundWindow(hwnd); Thread.Sleep(150);
        }
        if(GetForegroundWindow()!=hwnd) throw new InvalidOperationException("Could not foreground acceptance window");
        expectedInputWindow=hwnd;
        GetWindowThreadProcessId(hwnd, out expectedProcessId);
    }
    public static void RequireInputForeground() {
        uint actual;
        GetWindowThreadProcessId(GetForegroundWindow(), out actual);
        if(expectedInputWindow==IntPtr.Zero || actual!=expectedProcessId)
            throw new InvalidOperationException("Acceptance application lost foreground; refusing input");
    }
    public static void Key(ushort vk, bool up) {
        INPUT input = new INPUT(); input.type=1; input.u.ki.vk=vk; input.u.ki.flags=up ? 2u : 0u; Send(input);
    }
    public static void Press(ushort vk) {
        if(vk!=0x12) RequireInputForeground();
        Key(vk, false); Key(vk, true);
    }
    public static void Chord(ushort modifier, ushort vk) {
        RequireInputForeground();
        Key(modifier, false); try { Press(vk); } finally { Key(modifier, true); }
    }
    public static void Type(string text) { TypeText(text, false); }
    public static void TypeSource(string text) { TypeText(text, true); }
    static void TypeText(string text, bool literalNewlines) {
        RequireInputForeground();
        foreach (char ch in text) {
            // Source's ordinary Return creates a paragraph; Shift+Return is
            // its production soft-break path for one literal Markdown newline.
            if (ch == '\n') {
                if(literalNewlines) Chord(0x10, 0x0D); else Press(0x0D);
                continue;
            }
            if (ch == '\r') continue;
            RequireInputForeground();
            INPUT input = new INPUT(); input.type=1; input.u.ki.scan=ch; input.u.ki.flags=4; Send(input);
            input.u.ki.flags=6; Send(input);
        }
    }
    public static void Click(AutomationElement element) {
        RequireInputForeground();
        var current=Bounded(delegate() { return element.Current; }, "click target bounds");
        if (!current.IsEnabled || current.IsOffscreen || current.BoundingRectangle.IsEmpty)
            throw new InvalidOperationException("Cannot click inaccessible control: " + current.Name);
        var rect=current.BoundingRectangle;
        MovePhysical((int)(rect.Left+rect.Width/2), (int)(rect.Top+rect.Height/2));
        RequireInputForeground();
        INPUT input=new INPUT(); input.type=0; input.u.mi.dwFlags=2; Send(input);
        input.u.mi.dwFlags=4; Send(input); Thread.Sleep(200);
    }
    // A provider may block a single UIA call. Keep that call on a background
    // MTA thread so a broken accessibility branch cannot hang the whole gate.
    static T Bounded<T>(Func<T> action, string operation) {
        T result=default(T); Exception failure=null;
        var worker=new Thread(delegate() {
            try { result=action(); } catch(Exception error) { failure=error; }
        });
        worker.IsBackground=true; worker.SetApartmentState(ApartmentState.MTA); worker.Start();
        if(!worker.Join(6000)) throw new TimeoutException("UIA provider timed out during " + operation);
        if(failure!=null) throw new InvalidOperationException("UIA " + operation + " failed", failure);
        return result;
    }
    public static AutomationElement Root(int pid) {
        return Bounded(delegate() {
            return AutomationElement.RootElement.FindFirst(TreeScope.Children,
                new PropertyCondition(AutomationElement.ProcessIdProperty, pid));
        }, "application root discovery");
    }
    public static AutomationElement ForegroundRoot(int pid) {
        return Bounded(delegate() {
            var element=AutomationElement.FromHandle(GetForegroundWindow());
            return element.Current.ProcessId == pid ? element : null;
        }, "foreground dialog discovery");
    }
    struct Entry {
        public AutomationElement element; public int depth;
        public Entry(AutomationElement e, int d) { element=e; depth=d; }
    }
    static bool BrowserBranch(AutomationElement.AutomationElementInformation c) {
        // Menu/toolbar/Source searches need no Chromium accessibility subtree.
        return c.ControlType == ControlType.Document ||
            Regex.IsMatch(c.ClassName ?? "", "WebEngine|Chrome_|RenderWidget", RegexOptions.IgnoreCase) ||
            Regex.IsMatch(c.Name ?? "", "^(Live Markdown editor|Publishing preview|Web content)$", RegexOptions.IgnoreCase);
    }
    static AutomationElement Walk(AutomationElement root, string pattern, List<string> description, Stopwatch budget = null) {
        if(root==null) return null;
        var pending=new Queue<Entry>(); pending.Enqueue(new Entry(root, 0));
        var runtimeIds=new Dictionary<string, bool>();
        var elapsed=budget ?? Stopwatch.StartNew(); int visited=0;
        while(pending.Count>0 && visited<1600 && elapsed.ElapsedMilliseconds<4500) {
            var entry=pending.Dequeue(); visited++;
            try {
                var runtime=entry.element.GetRuntimeId();
                string runtimeId=String.Join(".", Array.ConvertAll<int, string>(runtime, delegate(int part) { return part.ToString(); }));
                if(runtimeIds.ContainsKey(runtimeId)) continue;
                runtimeIds.Add(runtimeId, true);
                var c=entry.element.Current;
                if(description!=null) description.Add("depth=" + entry.depth + " | " + c.ControlType.ProgrammaticName +
                    " | " + c.Name + " | " + c.AutomationId + " | " + c.ClassName + " | " +
                    c.BoundingRectangle + " | offscreen=" + c.IsOffscreen + " | enabled=" + c.IsEnabled);
                if(pattern!=null && !c.IsOffscreen && c.IsEnabled && !c.BoundingRectangle.IsEmpty &&
                    Regex.IsMatch(c.Name.Replace("&", ""), pattern)) return entry.element;
                if(entry.depth>=18 || BrowserBranch(c)) {
                    if(description!=null) description.Add("branch omitted: depth limit or embedded browser content");
                    continue;
                }
                // Crucially, never ask a provider for all descendants at once.
                var children=entry.element.FindAll(TreeScope.Children, Condition.TrueCondition);
                foreach(AutomationElement child in children) pending.Enqueue(new Entry(child, entry.depth+1));
            } catch(ElementNotAvailableException) {
                if(description!=null) description.Add("depth=" + entry.depth + " | unavailable branch");
            }
        }
        if(pattern!=null && pending.Count>0)
            throw new InvalidOperationException("UIA search incomplete; cannot establish target absence: " + pattern);
        if(description!=null && pending.Count>0) description.Add("bounded observation ended with " + pending.Count + " queued branches");
        return null;
    }
    static T RetryDisconnected<T>(Func<T> search, Action reacquire, Stopwatch budget, int budgetMs = 4500) {
        for(int retry=0; ; retry++) {
            if(budget.ElapsedMilliseconds>=budgetMs) throw new TimeoutException("UIA search retry budget expired");
            try {
                T found=search();
                if(budget.ElapsedMilliseconds>=budgetMs) throw new TimeoutException("UIA search retry budget expired");
                return found;
            } catch(COMException error) {
                // Popup publication or picker teardown can invalidate a provider traversal.
                // E_FAIL was observed in FindAll after opening File on x64.
                // Discard the entire read, reacquire the same owned HWND and
                // retry at most twice; never turn incomplete traversal into absence.
                if((error.ErrorCode!=unchecked((int)0x80010108) &&
                    error.ErrorCode!=unchecked((int)0x80004005)) || retry>=2) throw;
                Console.Error.WriteLine("UIA provider read retry {0}: HRESULT 0x{1:X8}", retry+1, unchecked((uint)error.ErrorCode));
                if(budget.ElapsedMilliseconds+100>=budgetMs) throw new TimeoutException("UIA search retry budget expired", error);
                Thread.Sleep(100);
                reacquire(); // Failure or a changed owner/handle remains fatal.
            }
        }
    }
    static AutomationElement ReacquireOwnedRoot(IntPtr handle, uint owner) {
        uint actual;
        if(handle==IntPtr.Zero || owner==0 || owner!=expectedProcessId ||
            GetWindowThreadProcessId(handle, out actual)==0 || actual!=owner)
            throw new InvalidOperationException("UIA search root no longer belongs to the owned application");
        var root=AutomationElement.FromHandle(handle);
        var current=root.Current;
        if(current.ProcessId!=owner || new IntPtr(current.NativeWindowHandle)!=handle)
            throw new InvalidOperationException("UIA reacquired a different root identity");
        return root;
    }
    public static AutomationElement Find(AutomationElement root, string pattern) {
        return Bounded(delegate() {
            if(root==null) return null;
            var current=root.Current;
            var handle=new IntPtr(current.NativeWindowHandle);
            uint owner=(uint)current.ProcessId;
            var budget=Stopwatch.StartNew();
            root=ReacquireOwnedRoot(handle, owner);
            return RetryDisconnected(delegate() { return Walk(root, pattern, null, budget); },
                delegate() { root=ReacquireOwnedRoot(handle, owner); }, budget);
        }, "bounded control search: " + pattern);
    }
    public static string RequireOwnedMenuPopup(AutomationElement item, int pid) {
        return Bounded(delegate() {
            var current=item.Current;
            if(pid<=0 || (uint)pid!=expectedProcessId || current.ProcessId!=pid ||
                current.ControlType!=ControlType.MenuItem || !current.IsEnabled ||
                current.IsOffscreen || current.BoundingRectangle.IsEmpty)
                throw new InvalidOperationException("File menu item is not visible and owned");
            var parent=TreeWalker.ControlViewWalker.GetParent(item);
            if(parent==null) throw new InvalidOperationException("File menu popup is missing");
            var popup=parent.Current;
            var bounds=current.BoundingRectangle;
            if(popup.ProcessId!=pid || popup.ControlType!=ControlType.Menu ||
                popup.ClassName!="QQuickPopupItem" || popup.IsOffscreen ||
                !popup.BoundingRectangle.Contains(bounds.Left+bounds.Width/2, bounds.Top+bounds.Height/2))
                throw new InvalidOperationException("File menu popup is not visible and owned");
            return "pid="+pid+" | popup="+popup.BoundingRectangle+" | item="+current.Name+" | bounds="+bounds;
        }, "owned File popup before keyboard scrolling");
    }
    // Retain immutable identity, not a row/COM object that scrolling can hide.
    // Every crop reacquires only this same popup under the same owned HWND.
    public sealed class OwnedMenuPopupIdentity {
        internal readonly int processId;
        internal readonly IntPtr window;
        internal readonly string windowRuntime, windowClass, popupRuntime;
        internal OwnedMenuPopupIdentity(int pid, IntPtr hwnd, string rootId, string rootClass, string popupId) {
            processId=pid; window=hwnd; windowRuntime=rootId; windowClass=rootClass; popupRuntime=popupId;
        }
        public override string ToString() {
            return "pid="+processId+" | window="+window+" | windowRuntime="+windowRuntime+
                " | windowClass="+windowClass+" | popupRuntime="+popupRuntime+" | popupClass=QQuickPopupItem";
        }
    }
    static string MenuRuntimeId(AutomationElement element) {
        var id=element.GetRuntimeId();
        if(id==null || id.Length==0) throw new InvalidOperationException("Missing File popup/window runtime identity");
        return String.Join(".", Array.ConvertAll<int,string>(id, delegate(int part) { return part.ToString(); }));
    }
    public static OwnedMenuPopupIdentity RetainOwnedMenuPopup(AutomationElement item, int pid) {
        RequireOwnedMenuPopup(item,pid);
        return Bounded(delegate() {
            var popup=TreeWalker.ControlViewWalker.GetParent(item);
            if(popup==null) throw new InvalidOperationException("Proved File popup disappeared");
            var parent=TreeWalker.ControlViewWalker.GetParent(popup);
            if(parent==null) throw new InvalidOperationException("File popup has no owning window parent");
            var window=parent.Current;
            if(window.ProcessId!=pid || window.ControlType!=ControlType.Window ||
                new IntPtr(window.NativeWindowHandle)!=expectedInputWindow)
                throw new InvalidOperationException("File popup parent is not the owned application window");
            var proof=new OwnedMenuPopupIdentity(pid,expectedInputWindow,MenuRuntimeId(parent),window.ClassName,MenuRuntimeId(popup));
            // Revalidate the captured identity before returning; no row is
            // consulted again after this initial proof or the wheel gesture.
            OwnedPopupBounds(proof,pid);
            return proof;
        }, "retain proved File popup identity");
    }
    static System.Windows.Rect OwnedPopupBounds(OwnedMenuPopupIdentity proof, int pid) {
        if(proof==null || pid!=proof.processId || (uint)pid!=expectedProcessId || proof.window!=expectedInputWindow)
            throw new InvalidOperationException("Retained File popup owner/window changed");
        var root=ReacquireOwnedRoot(proof.window,(uint)pid);
        var window=root.Current;
        if(window.ControlType!=ControlType.Window || window.ClassName!=proof.windowClass ||
            MenuRuntimeId(root)!=proof.windowRuntime || window.IsOffscreen || !window.IsEnabled)
            throw new InvalidOperationException("Retained File popup window identity/state changed");
        AutomationElement found=null;
        foreach(AutomationElement child in root.FindAll(TreeScope.Children,Condition.TrueCondition)) {
            if(MenuRuntimeId(child)!=proof.popupRuntime) continue;
            if(found!=null) throw new InvalidOperationException("Retained File popup identity is not unique");
            found=child;
        }
        if(found==null) throw new InvalidOperationException("Retained File popup is no longer present");
        var popup=found.Current;
        var parent=TreeWalker.ControlViewWalker.GetParent(found);
        if(popup.ProcessId!=pid || popup.ControlType!=ControlType.Menu || popup.ClassName!="QQuickPopupItem" ||
            popup.IsOffscreen || !popup.IsEnabled || popup.BoundingRectangle.IsEmpty ||
            parent==null || MenuRuntimeId(parent)!=proof.windowRuntime ||
            !window.BoundingRectangle.Contains(popup.BoundingRectangle))
            throw new InvalidOperationException("Retained File popup parent/class/ownership/bounds changed");
        return popup.BoundingRectangle;
    }
    public static System.Windows.Rect OwnedMenuBounds(OwnedMenuPopupIdentity proof, int pid) {
        return Bounded(delegate(){return OwnedPopupBounds(proof,pid);},"current retained File popup bounds");
    }
    public static string ScrollOwnedMenuDown(AutomationElement item, int pid) {
        string proof=RequireOwnedMenuPopup(item,pid);
        var bounds=Bounded(delegate(){return item.Current.BoundingRectangle;},"owned menu wheel position");
        RequireInputForeground();
        MovePhysical((int)(bounds.Left+bounds.Width/2),(int)(bounds.Top+bounds.Height/2));
        RequireInputForeground();
        INPUT input=new INPUT();input.type=0;input.u.mi.dwFlags=0x0800;
        input.u.mi.mouseData=unchecked((uint)-360);Send(input);
        return proof+" | physical wheel delta=-360";
    }
    public static System.Windows.Rect OwnedMenuBounds(AutomationElement item, int pid) {
        RequireOwnedMenuPopup(item,pid);
        return Bounded(delegate(){return TreeWalker.ControlViewWalker.GetParent(item).Current.BoundingRectangle;},"owned File popup bounds");
    }
    public static void ClickOwnedMenuPoint(AutomationElement anchor, int pid, int x, int y, System.Windows.Rect measuredPopup, long capturedAt) {
        var popup=OwnedMenuBounds(anchor,pid);
        if(popup!=measuredPopup || !popup.Contains(x,y))
            throw new InvalidOperationException("OCR menu target or popup geometry changed");
        double ageMs=(Stopwatch.GetTimestamp()-capturedAt)*1000.0/Stopwatch.Frequency;
        if(ageMs<0 || ageMs>750) throw new InvalidOperationException("Final menu crop is no longer current; refusing input");
        RequireInputForeground();MovePhysical(x,y);RequireInputForeground();
        INPUT input=new INPUT();input.type=0;input.u.mi.dwFlags=2;Send(input);
        input.u.mi.dwFlags=4;Send(input);
    }
    public static void ClickOwnedMenuPoint(OwnedMenuPopupIdentity anchor, int pid, int x, int y, System.Windows.Rect measuredPopup, long capturedAt) {
        var popup=OwnedMenuBounds(anchor,pid);
        if(popup!=measuredPopup || !popup.Contains(x,y))
            throw new InvalidOperationException("OCR menu target or popup geometry changed");
        double ageMs=(Stopwatch.GetTimestamp()-capturedAt)*1000.0/Stopwatch.Frequency;
        if(ageMs<0 || ageMs>750) throw new InvalidOperationException("Final menu crop is no longer current; refusing input");
        RequireInputForeground();MovePhysical(x,y);RequireInputForeground();
        INPUT input=new INPUT();input.type=0;input.u.mi.dwFlags=2;Send(input);
        input.u.mi.dwFlags=4;Send(input);
    }
    public static bool OpenedDocument(AutomationElement main, IntPtr pickerHandle, string title) {
        return Bounded(delegate() {
            if(pickerHandle==IntPtr.Zero) throw new InvalidOperationException("Missing native picker identity");
            if(IsWindow(pickerHandle)) return false;
            var current=main.Current;
            var owned=ReacquireOwnedRoot(new IntPtr(current.NativeWindowHandle), (uint)current.ProcessId);
            return owned.Current.Name==title;
        }, "opened document identity");
    }
    public static bool Checked(AutomationElement element) {
        return Bounded(delegate() { return CheckedValue(element); }, "checked state readback");
    }
    static bool CheckedValue(AutomationElement element) {
        object pattern;
        if(element.TryGetCurrentPattern(TogglePattern.Pattern, out pattern))
            return ((TogglePattern)pattern).Current.ToggleState == ToggleState.On;
        if(element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out pattern))
            return ((SelectionItemPattern)pattern).Current.IsSelected;
        throw new InvalidOperationException("Control exposes no checked/selected state: " + element.Current.Name);
    }
    public static string Text(AutomationElement element) {
        return Bounded(delegate() { return TextValue(element); }, "Source text readback");
    }
    static string TextValue(AutomationElement element) {
        object pattern;
        if(element.TryGetCurrentPattern(ValuePattern.Pattern, out pattern)) return ((ValuePattern)pattern).Current.Value;
        if(element.TryGetCurrentPattern(TextPattern.Pattern, out pattern)) return ((TextPattern)pattern).DocumentRange.GetText(-1);
        throw new InvalidOperationException("Editor exposes no readable text pattern");
    }
    public static string[] Describe(AutomationElement root) {
        var result=new List<string>();
        try {
            return Bounded(delegate() { Walk(root, null, result); return result.ToArray(); }, "bounded diagnostic observation");
        } catch(Exception error) {
            // Diagnostics never change a required workflow assertion's result.
            // Preserve whatever the worker observed, even if a provider stalled.
            return new string[] { "UIA diagnostic incomplete: " + error.Message };
        }
    }
}
