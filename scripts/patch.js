// ==Discord Screenshare Linux Fix==
try {
    const lib = require('./linux-fix.node');
    lib.init(dataDirectory);
    lib.inject();
} catch (e) {
    console.log("Linux fix error", e);
}
// ==Discord Screenshare Linux Fix==