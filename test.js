let lib;
try {
    lib = require('./build/Debug/discord-pawgers.node')
    console.log("Running in debug mode");
} catch (e) {
    //console.log(e)
    lib = require('./build/Release/discord-pawgers.node')
    console.log("Running in release mode");
}
console.log(lib);

var process = require('process');
console.log("PID:", process.pid)

const keypress = async () => {
    process.stdin.setRawMode(true)
    return new Promise(resolve => process.stdin.once('data', () => {
      process.stdin.setRawMode(false)
      resolve()
    }))
}

(async () => {
    console.log("Press any key to run...");
    await keypress()
    lib.init();
    console.log(lib.inject(884826))
    //setTimeout(() => {}, 200000000);
})()

