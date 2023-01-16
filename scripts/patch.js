// ==Discord Screenshare Linux Fix==
features.declareSupported('soundshare');
try {
    const lib = require('./linux-fix.node');
    lib.init(dataDirectory);
    lib.inject();
} catch (e) {
    console.log("Linux fix error", e);
}
// ==Discord Screenshare Linux Fix==