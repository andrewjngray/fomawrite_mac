// OS input and read-only native/UIA witnesses, scoped to the ordinary gate PID.
public static class DesktopSupplementInput {
    public sealed class PaletteState {
        public int processId; public long hwnd; public bool present,ready,keyboardFocus;
        public string queryAutomationId,observedUtc;
    }
    public static PaletteState Palette(System.IntPtr window,int pid) {
        PaletteState result=null;System.Exception failure=null;
        var worker=new System.Threading.Thread(delegate(){try{
            Owner(window,pid);
            var root=System.Windows.Automation.AutomationElement.FromHandle(window);
            if(root==null||root.Current.ProcessId!=pid||root.Current.NativeWindowHandle!=window.ToInt32())
                throw new System.InvalidOperationException("Palette observation lost its owned main HWND");
            result=new PaletteState{processId=pid,hwnd=window.ToInt64(),observedUtc=System.DateTime.UtcNow.ToString("o")};
            int dialogs=0,queries=0;
            foreach(System.Windows.Automation.AutomationElement dialog in root.FindAll(System.Windows.Automation.TreeScope.Children,System.Windows.Automation.Condition.TrueCondition)) {
                var d=dialog.Current;
                if(d.ControlType!=System.Windows.Automation.ControlType.Window||d.Name!="Command Palette"||d.IsOffscreen)continue;
                if(d.ProcessId!=pid||++dialogs!=1)throw new System.InvalidOperationException("Palette dialog ownership or uniqueness differs");
                result.present=true;
                foreach(System.Windows.Automation.AutomationElement query in dialog.FindAll(System.Windows.Automation.TreeScope.Children,System.Windows.Automation.Condition.TrueCondition)) {
                    var q=query.Current;
                    if(q.ControlType!=System.Windows.Automation.ControlType.Edit||!(q.AutomationId??"").EndsWith(".commandQuery",System.StringComparison.Ordinal))continue;
                    if(q.ProcessId!=pid||++queries!=1||q.IsOffscreen||!q.IsEnabled||q.BoundingRectangle.IsEmpty||!d.BoundingRectangle.Contains(q.BoundingRectangle))
                        throw new System.InvalidOperationException("Palette query is not unique, visible and owned");
                    result.queryAutomationId=q.AutomationId;result.keyboardFocus=q.HasKeyboardFocus;result.ready=q.HasKeyboardFocus;
                }
            }
            Owner(window,pid);
        }catch(System.Exception error){failure=error;}});
        worker.IsBackground=true;worker.SetApartmentState(System.Threading.ApartmentState.MTA);worker.Start();
        if(!worker.Join(6000))throw new System.TimeoutException("Palette provider read timed out");
        if(failure!=null)throw new System.InvalidOperationException("Palette read failed",failure);return result;
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    struct Rect { public int left,top,right,bottom; }
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    struct MonitorInfo { public int size; public Rect monitor,work; public uint flags; }
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(System.IntPtr window,out uint pid);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern bool GetWindowRect(System.IntPtr window,out Rect rect);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern bool IsIconic(System.IntPtr window);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern bool IsZoomed(System.IntPtr window);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern System.IntPtr MonitorFromWindow(System.IntPtr window,uint flags);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern bool GetMonitorInfo(System.IntPtr monitor,ref MonitorInfo info);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern System.IntPtr GetKeyboardLayout(uint thread);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern short GetAsyncKeyState(int key);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern System.IntPtr SendMessageTimeout(System.IntPtr window,uint message,System.UIntPtr wparam,System.IntPtr lparam,uint flags,uint timeout,out System.UIntPtr result);
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    struct Mouse { public int x,y;public uint data,flags,time;public System.UIntPtr extra; }
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit)]
    struct Union { [System.Runtime.InteropServices.FieldOffset(0)] public Mouse mouse; }
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    struct Input { public uint type;public Union u; }
    [System.Runtime.InteropServices.DllImport("user32.dll",SetLastError=true)] static extern uint SendInput(uint count,Input[] inputs,int size);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern uint GetDpiForWindow(System.IntPtr window);
    [System.Runtime.InteropServices.DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(System.IntPtr window,uint attribute,out Rect rect,int size);
    public sealed class WindowState {
        public int processId,left,top,width,height,workLeft,workTop,workWidth,workHeight;
        public long hwnd,monitor;
        public string keyboardLayout,observedUtc;
        public bool minimized,maximized;
        public uint dpi; public int visibleFrameHresult,visibleLeft,visibleTop,visibleWidth,visibleHeight;
    }
    static uint Owner(System.IntPtr window,int pid) {
        uint owner;uint thread=GetWindowThreadProcessId(window,out owner);
        if(window==System.IntPtr.Zero||thread==0||owner!=(uint)pid) throw new System.InvalidOperationException("Owned window identity changed");
        return thread;
    }
    public static WindowState Window(System.IntPtr window,int pid) {
        uint thread=Owner(window,pid); Rect rect;
        var monitor=MonitorFromWindow(window,2);var info=new MonitorInfo();info.size=System.Runtime.InteropServices.Marshal.SizeOf(typeof(MonitorInfo));
        if(!GetWindowRect(window,out rect)||monitor==System.IntPtr.Zero||!GetMonitorInfo(monitor,ref info)) throw new System.InvalidOperationException("Native window/monitor geometry unavailable");
        Rect visible; int frameResult=DwmGetWindowAttribute(window,9,out visible,System.Runtime.InteropServices.Marshal.SizeOf(typeof(Rect)));
        return new WindowState {dpi=GetDpiForWindow(window),visibleFrameHresult=frameResult,visibleLeft=visible.left,visibleTop=visible.top,visibleWidth=visible.right-visible.left,visibleHeight=visible.bottom-visible.top,processId=pid,hwnd=window.ToInt64(),monitor=monitor.ToInt64(),left=rect.left,top=rect.top,width=rect.right-rect.left,height=rect.bottom-rect.top,
            workLeft=info.work.left,workTop=info.work.top,workWidth=info.work.right-info.work.left,workHeight=info.work.bottom-info.work.top,
            minimized=IsIconic(window),maximized=IsZoomed(window),keyboardLayout=(GetKeyboardLayout(thread).ToInt64()&0xffffffffL).ToString("x8"),observedUtc=System.DateTime.UtcNow.ToString("o")};
    }
    public static void Chord(System.IntPtr window,int pid,ushort[] modifiers,ushort key,bool usPunctuation) {
        uint thread=Owner(window,pid);NormalLaunchInput.RequireInputForeground();
        if(NormalLaunchInput.GetForegroundWindow()!=window) throw new System.InvalidOperationException("Shortcut requires the exact owned main foreground HWND");
        if(usPunctuation&&(GetKeyboardLayout(thread).ToInt64()&0xffffffffL)!=0x04090409L) throw new System.InvalidOperationException("US OEM_1 qualification requires unchanged standard US 04090409 layout");
        foreach(int modifier in new int[]{0x10,0x11,0x12,0x5B,0x5C})
            if((GetAsyncKeyState(modifier)&0x8000)!=0) throw new System.InvalidOperationException("Held modifier; no shortcut input/replay permitted");
        int pressed=0;
        try { foreach(ushort modifier in modifiers) {NormalLaunchInput.RequireInputForeground();NormalLaunchInput.Key(modifier,false);pressed++;} NormalLaunchInput.Press(key); }
        finally {while(pressed>0)NormalLaunchInput.Key(modifiers[--pressed],true);}
    }
    public static void OffsetByTitlebarDrag(System.IntPtr window,int pid) {
        var state=Window(window,pid);NormalLaunchInput.RequireInputForeground();
        if(state.minimized||state.maximized||NormalLaunchInput.GetForegroundWindow()!=window) throw new System.InvalidOperationException("Titlebar drag requires exact owned ordinary window");
        int x=state.left+System.Math.Min(160,state.width/2),y=0;
        for(int offset=6;offset<=48;offset+=6) {
            System.UIntPtr hit;int probeY=state.top+offset;
            if(SendMessageTimeout(window,0x84,System.UIntPtr.Zero,new System.IntPtr((x&0xffff)|((probeY&0xffff)<<16)),2,600,out hit)==System.IntPtr.Zero)
                throw new System.TimeoutException("Owned nonclient hit-test unavailable");
            if(hit.ToUInt64()==2){y=probeY;break;} // HTCAPTION, read-only witness.
        }
        if(y==0)throw new System.InvalidOperationException("No verified native titlebar point; no guessed drag");
        int dx=state.workLeft+System.Math.Max(24,(state.workWidth-state.width)/4)-state.left;
        int dy=state.workTop+40-state.top;
        if(dx==0&&dy==0)dx=40;
        NormalLaunchInput.MovePhysical(x,y);NormalLaunchInput.RequireInputForeground();
        var down=new Input();down.u.mouse.flags=2;var up=down;up.u.mouse.flags=4;
        bool pressed=false;
        try {
            if(SendInput(1,new Input[]{down},System.Runtime.InteropServices.Marshal.SizeOf(typeof(Input)))!=1)throw new System.InvalidOperationException("Titlebar mouse down rejected; no replay");
            pressed=true;NormalLaunchInput.MovePhysical(x+dx,y+dy);System.Threading.Thread.Sleep(150);
        } finally {if(pressed&&SendInput(1,new Input[]{up},System.Runtime.InteropServices.Marshal.SizeOf(typeof(Input)))!=1)throw new System.InvalidOperationException("Titlebar mouse release rejected; stop native gate");}
    }
    public sealed class SelectionState { public int processId,rangeCount; public bool keyboardFocus; public string text,observedUtc; }
    public static SelectionState Selection(System.Windows.Automation.AutomationElement source,int pid) {
        SelectionState result=null;System.Exception failure=null;
        var worker=new System.Threading.Thread(delegate(){try{
            var state=source.Current;
            if(state.ProcessId!=pid||state.ControlType!=System.Windows.Automation.ControlType.Edit||state.Name!="Markdown editor"||state.IsOffscreen)
                throw new System.InvalidOperationException("Selection witness is not the owned visible Source editor");
            object raw;if(!source.TryGetCurrentPattern(System.Windows.Automation.TextPattern.Pattern,out raw)) throw new System.InvalidOperationException("Source TextPattern selection unavailable; native qualification remains pending");
            var ranges=((System.Windows.Automation.TextPattern)raw).GetSelection();
            if(ranges.Length!=1)throw new System.InvalidOperationException("Expected exactly one real Source selection");
            result=new SelectionState {processId=pid,rangeCount=ranges.Length,keyboardFocus=state.HasKeyboardFocus,text=ranges[0].GetText(256),observedUtc=System.DateTime.UtcNow.ToString("o")};
        }catch(System.Exception error){failure=error;}});
        worker.IsBackground=true;worker.SetApartmentState(System.Threading.ApartmentState.MTA);worker.Start();
        if(!worker.Join(6000))throw new System.TimeoutException("Selection provider read timed out");
        if(failure!=null)throw new System.InvalidOperationException("Selection read failed",failure);return result;
    }
}

// Disposable OS-input harness only. Provider reads return primitive snapshots;
// no UIA Invoke/SetFocus, app command, browser script, clipboard or key replay.
public static class OwnedMenuNavigation {
    public sealed class PopupState {
        public int processId,itemPid,popupPid,windowPid;
        public long windowHwnd;
        public string state,itemName,itemBounds,popupBounds,popupClass,observedUtc,errorType;
        public bool ownerVerified,itemOffscreen,popupOffscreen,itemEnabled,geometryContained;
    }
    public sealed class HoverState {
        public int processId; public long windowHwnd;
        public string name,bounds,observedUtc;
        public bool ownerVerified,physicalClickSent;
    }
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern bool IsWindow(System.IntPtr hwnd);
    [System.Runtime.InteropServices.DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(System.IntPtr hwnd,out uint pid);
    static void WindowOwner(System.IntPtr hwnd,int pid) {
        uint owner;GetWindowThreadProcessId(hwnd,out owner);
        if(hwnd==System.IntPtr.Zero||pid<=0||!IsWindow(hwnd)||owner!=(uint)pid)
            throw new System.InvalidOperationException("Owned menu window identity changed");
    }
    static T Read<T>(System.Func<T> action) {
        T result=default(T);System.Exception failure=null;
        var worker=new System.Threading.Thread(delegate(){try{result=action();}catch(System.Exception e){failure=e;}});
        worker.IsBackground=true;worker.SetApartmentState(System.Threading.ApartmentState.MTA);worker.Start();
        if(!worker.Join(6000))throw new System.TimeoutException("Owned menu snapshot provider timed out");
        if(failure!=null)throw new System.InvalidOperationException("Owned menu snapshot failed",failure);
        return result;
    }
    public static PopupState Popup(System.Windows.Automation.AutomationElement item,int pid,System.IntPtr window,string expectedName) {
        return Read(delegate(){
            WindowOwner(window,pid);
            var state=new PopupState {processId=pid,windowPid=pid,windowHwnd=window.ToInt64(),observedUtc=System.DateTime.UtcNow.ToString("o"),state="provider-unavailable"};
            try {
                var c=item.Current;
                state.itemPid=c.ProcessId;state.itemName=(c.Name??"").Replace("&","");
                if(c.ProcessId!=pid||c.ControlType!=System.Windows.Automation.ControlType.MenuItem||state.itemName!=expectedName)
                    throw new System.InvalidOperationException("Popup item owner/type/name differs");
                state.itemBounds=c.BoundingRectangle.ToString();state.itemOffscreen=c.IsOffscreen;state.itemEnabled=c.IsEnabled;
                var parent=System.Windows.Automation.TreeWalker.ControlViewWalker.GetParent(item);
                if(parent==null){state.errorType="MissingParent";return state;}
                var p=parent.Current;state.popupPid=p.ProcessId;state.popupClass=p.ClassName;state.popupBounds=p.BoundingRectangle.ToString();state.popupOffscreen=p.IsOffscreen;
                if(p.ProcessId!=pid||p.ControlType!=System.Windows.Automation.ControlType.Menu||p.ClassName!="QQuickPopupItem")
                    throw new System.InvalidOperationException("Popup parent owner/type/class differs");
                WindowOwner(window,pid);state.ownerVerified=true;
                var rect=c.BoundingRectangle;var popup=p.BoundingRectangle;
                state.geometryContained=!rect.IsEmpty&&!popup.IsEmpty&&rect.Width>0&&rect.Height>0&&popup.Width>0&&popup.Height>0&&popup.Contains(rect.Left+rect.Width/2,rect.Top+rect.Height/2);
                state.state=(!state.itemOffscreen&&!state.popupOffscreen&&state.geometryContained)?"visible":"owned-transition";
                return state;
            } catch(System.Windows.Automation.ElementNotAvailableException) {
                // Unavailability is observation-only. It never proves absence
                // or authorizes input; a complete new search must do that.
                state.ownerVerified=false;state.state="provider-unavailable";state.errorType="ElementNotAvailableException";return state;
            }
        });
    }
    static HoverState Hover(System.Windows.Automation.AutomationElement element,int pid,System.IntPtr window,string name,bool source) {
        double[] point=null;
        var result=Read(delegate(){
            WindowOwner(window,pid);var c=element.Current;
            var kind=source?System.Windows.Automation.ControlType.Edit:System.Windows.Automation.ControlType.MenuBar;
            if(c.ProcessId!=pid||c.ControlType!=kind||(c.Name??"").Replace("&","")!=name||c.IsOffscreen||!c.IsEnabled||c.BoundingRectangle.IsEmpty)
                throw new System.InvalidOperationException("Hover target is not the named owned visible control");
            var root=System.Windows.Automation.AutomationElement.FromHandle(window).Current;
            if(root.ProcessId!=pid||!root.BoundingRectangle.Contains(c.BoundingRectangle))
                throw new System.InvalidOperationException("Hover target escaped the exact owned window");
            var rect=c.BoundingRectangle;point=new double[]{rect.Left+rect.Width/2,rect.Top+rect.Height*(source?0.75:0.5)};
            return new HoverState {processId=pid,windowHwnd=window.ToInt64(),name=name,bounds=rect.ToString(),ownerVerified=true,physicalClickSent=false,observedUtc=System.DateTime.UtcNow.ToString("o")};
        });
        WindowOwner(window,pid);NormalLaunchInput.RequireInputForeground();NormalLaunchInput.MovePhysical((int)point[0],(int)point[1]);NormalLaunchInput.RequireInputForeground();
        return result;
    }
    public static HoverState HoverTop(System.Windows.Automation.AutomationElement element,int pid,System.IntPtr window,string name) {return Hover(element,pid,window,name,false);}
    public static HoverState HoverSource(System.Windows.Automation.AutomationElement element,int pid,System.IntPtr window) {return Hover(element,pid,window,"Markdown editor",true);}
    public static bool SourceFocused(System.Windows.Automation.AutomationElement element,int pid,System.IntPtr window) {
        return Read(delegate(){WindowOwner(window,pid);var c=element.Current;
            if(c.ProcessId!=pid||c.ControlType!=System.Windows.Automation.ControlType.Edit||c.Name!="Markdown editor")
                throw new System.InvalidOperationException("Source focus observation escaped the owned editor");
            return !c.IsOffscreen&&c.HasKeyboardFocus;
        });
    }
}
