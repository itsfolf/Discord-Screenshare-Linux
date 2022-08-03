console.log("hi");
debugger;
Interceptor.detachAll();
if (!Process.enumerateModules().find(x => x.name === 'libwebrtc.so')) {
    send('libwebrtc.so not found');
    Module.load('/home/folf/.config/discordcanary/0.0.136/modules/discord_voice/libwebrtc.so')
}
var exportsDiscord = Module.enumerateSymbolsSync("discord_voice.node");
var exportsRtc = Module.enumerateSymbolsSync("libwebrtc.so");
var rtcSyms = []
for (var i = 0; i < exportsRtc.length; i++) {
    if (exportsRtc[i].type == 'function') rtcSyms.push(exportsRtc[i]);
}
var rtcSymsNames = rtcSyms.filter(r => r && r.name && r.name.toLowerCase().includes("webrtc")).map(r => r.name);
send('Loaded ' + rtcSyms.length + ' symbols from libwebrtc.so (' + rtcSymsNames.length + ' functions)');
send('Base ' + Module.getBaseAddress('discord_voice.node'));

const createDefaultOpt = new NativeFunction(rtcSyms.find(x => x.name === "_ZN6webrtc21DesktopCaptureOptions13CreateDefaultEv").address, 'pointer', ['pointer']);
const origCreateScreenCapturer = new NativeFunction(rtcSyms.find(x => x.name === "_ZN6webrtc15DesktopCapturer20CreateScreenCapturerERKNS_21DesktopCaptureOptionsE").address, 'pointer', ['pointer', 'pointer']);
const origCreateCroppingCapturer = new NativeFunction(rtcSyms.find(x => x.name === "_ZN6webrtc22CroppingWindowCapturer14CreateCapturerERKNS_21DesktopCaptureOptionsE").address, 'pointer', ['pointer', 'pointer']);
const origDesktopAndCursorComposer = new NativeFunction(rtcSyms.find(x => x.name === "_ZN6webrtc24DesktopAndCursorComposerC2ESt10unique_ptrINS_15DesktopCapturerESt14default_deleteIS2_EERKNS_21DesktopCaptureOptionsE").address, 'pointer', ['pointer', 'pointer', 'pointer']);
let optsAddr;
for (var i = 0; i < exportsDiscord.length; i++) {
    if (exportsDiscord[i].type == 'function') {
        try {
            const name = exportsDiscord[i].name;
            if (name.includes('_ZN6webrtc15DesktopCapturer20CreateScreenCapturerERKNS_21DesktopCaptureOptionsE')) {
                send('Found ' + name);
                Interceptor.replace(exportsDiscord[i].address, new NativeCallback((target, opts) => {
                    send('Intercepted ' + name);
                    send('Ptr: ' + target);
                    if (!optsAddr) {
                            optsAddr = Memory.alloc(Process.pointerSize);
                            createDefaultOpt(optsAddr)
                            optsAddr.add(0x1B).writeU8(1);
                            send('Created options ' + optsAddr);
                    }
                    send('Using options: ' + optsAddr);
                    const c = origCreateScreenCapturer(target, optsAddr);
                    send('Created capturer at: ' + c + " " + target);
                    return c;
                }, 'pointer', ['pointer', 'pointer']));
            } else if (name.includes('_ZN6webrtc22CroppingWindowCapturer14CreateCapturerERKNS_21DesktopCaptureOptionsE')) {
                send('Found ' + name);
                Interceptor.attach(exportsDiscord[i].address, {
                    onEnter: function (args) {
                        send('Intercepted ' + name);
                        this.target = args[0];
                        send('Ptr: ' + this.target);
                    },
                    onLeave: function (retval) {
                        send('Returned ' + name);
                        if (!optsAddr) {
                            optsAddr = Memory.alloc(Process.pointerSize);
                            createDefaultOpt(optsAddr)
                            optsAddr.add(0x19).writeU8(1);
                            send('Created options ' + optsAddr);
                        }
                        send('Using options: ' + optsAddr);
                        this.cap = origCreateCroppingCapturer(this.target, optsAddr);
                        retval.replace(this.cap);
                        send('Created capturer at: ' + retval);
                    }
                });
            } else if (name.includes('_ZN6webrtc24DesktopAndCursorComposerC2ENSt3__110unique_ptrINS_15DesktopCapturerENS1_14default_deleteIS3_EEEERKNS_21DesktopCaptureOptionsE')) {
                send('Found ' + name);
                Interceptor.replace(exportsDiscord[i].address, new NativeCallback((target, capturer, opts) => {
                    send('Intercepted ' + name);
                    send('Ptr: ' + target);
                    send('Capturer: ' + capturer);
                    send('Using options: ' + optsAddr);
                    const c = origDesktopAndCursorComposer(target, capturer, optsAddr);
                    send('Created composer at: ' + c + " " + target);
                    return c;
                }, 'pointer', ['pointer', 'pointer', 'pointer']));
            }
        } catch (e) { send(e) }
    }
}
send('done')