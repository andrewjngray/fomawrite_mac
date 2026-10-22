// Appended to the existing guarded input helper in a disposable per-case copy.
// No UIA setter, JavaScript injection, application API or alternate input route.
public static class LiveEditInput {
    public sealed class Focus {
        public int processId; public string controlType; public string name;
        public bool keyboardFocus; public bool offscreen;
    }
    public sealed class RapidTiming {
        public int requestedGapMs; public long frequency, undoBegin, undoEnd, saveBegin, saveEnd;
        public string observedUtc;
    }
    static T Read<T>(System.Func<T> action) {
        T value=default(T); System.Exception error=null;
        var thread=new System.Threading.Thread(delegate() { try { value=action(); } catch(System.Exception e) { error=e; } });
        thread.IsBackground=true; thread.SetApartmentState(System.Threading.ApartmentState.MTA); thread.Start();
        if(!thread.Join(6000)) throw new System.TimeoutException("Live input observation timed out");
        if(error!=null) throw new System.InvalidOperationException("Live input observation failed", error);
        return value;
    }
    public static Focus ObserveFocus() {
        return Read(delegate() {
            var c=System.Windows.Automation.AutomationElement.FocusedElement.Current;
            return new Focus { processId=c.ProcessId, controlType=c.ControlType.ProgrammaticName,
                name=c.Name, keyboardFocus=c.HasKeyboardFocus, offscreen=c.IsOffscreen };
        });
    }
    public static string Title(int pid) {
        return NormalLaunchInput.OwnedWindowTitle(pid);
    }
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    struct Mouse { public int dx,dy; public uint data,flags,time; public System.UIntPtr extra; }
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit)]
    struct Union { [System.Runtime.InteropServices.FieldOffset(0)] public Mouse mouse; }
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    struct Input { public uint type; public Union u; }
    [System.Runtime.InteropServices.DllImport("user32.dll",SetLastError=true)]
    static extern uint SendInput(uint n, Input[] inputs, int size);
    public static void ClickWord(int pid, System.Windows.Rect pane, int x, int y) {
        NormalLaunchInput.RequireInputForeground();
        var window=NormalLaunchInput.OwnedWindowBounds(pid);
        if(pane.IsEmpty || !window.Contains(pane) || !pane.Contains(new System.Windows.Point(x,y)))
            throw new System.InvalidOperationException("OCR target lies outside the owned writing pane");
        NormalLaunchInput.MovePhysical(x,y);
        NormalLaunchInput.RequireInputForeground();
        var down=new Input(); down.type=0; down.u.mouse.flags=2;
        var up=down; up.u.mouse.flags=4;
        if(SendInput(2,new Input[]{down,up},System.Runtime.InteropServices.Marshal.SizeOf(typeof(Input)))!=2)
            throw new System.InvalidOperationException("Windows rejected Live text click");
        System.Threading.Thread.Sleep(200);
    }
    public static RapidTiming UndoThenSave(int gapMs) {
        if(gapMs!=0 && gapMs!=50 && gapMs!=100 && gapMs!=150)
            throw new System.ArgumentOutOfRangeException("gapMs");
        var t=new RapidTiming { requestedGapMs=gapMs, frequency=System.Diagnostics.Stopwatch.Frequency,
            observedUtc=System.DateTime.UtcNow.ToString("o") };
        t.undoBegin=System.Diagnostics.Stopwatch.GetTimestamp();
        NormalLaunchInput.Chord(0x11,0x5A);
        t.undoEnd=System.Diagnostics.Stopwatch.GetTimestamp();
        if(gapMs>0) {
            long deadline=t.undoEnd+(System.Diagnostics.Stopwatch.Frequency*gapMs/1000);
            System.Threading.Thread.Sleep(gapMs);
            while(System.Diagnostics.Stopwatch.GetTimestamp()<deadline) System.Threading.Thread.Sleep(1);
        }
        t.saveBegin=System.Diagnostics.Stopwatch.GetTimestamp();
        NormalLaunchInput.Chord(0x11,0x53);
        t.saveEnd=System.Diagnostics.Stopwatch.GetTimestamp();
        return t;
    }
}
