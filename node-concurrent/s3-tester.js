try {
var nc = require('nodeaffinity');

//Returns the cpus/cores (affinity mask) on which current node process is allowed to run
//Failure returns -1
console.log(nc.getAffinity());

//Sets process CPU affinity, here 3 means 011 i.e. process will be allowed to run on cpu0 and cpu1
// returns same mask id success , if failure retuen -1.
console.log(nc.setAffinity(4));
} catch(e) {console.log(e)}

var AWS = require("aws-sdk");
var performance = require('perf_hooks').performance

AWS.config.getCredentials(function(err) {
  if (err) console.log(err.stack);
  // credentials not loaded
  else {
    console.log("Access key:", AWS.config.credentials.accessKeyId);
  }
});

// Load the SDK and UUID
var uuid = require('uuid');

// Create unique bucket name
var bucketName = 'latency-throughput-test';
// Create name for uploaded object key
var keyName = 'test-file.txt';

var s3 = new AWS.S3({
    region: 'us-west-2'
});

async function testWrite(s3Params, startTime, workers, size) {
    let promises = []
    for (let i = 0; i < workers; i++) {
        promises.push(s3.putObject(s3Params).promise());
    }

    await Promise.all(promises);
    var endTime = performance.now();
    console.log("WRITE from " + bucketName + "/" + keyName, "in", endTime - startTime, "ms", workers, "workers", size, "size");
}

async function testRead(s3Params, startTime, workers, size) {
    let promises = []
    for (let i = 0; i < workers; i++) {
        promises.push(s3.getObject(s3Params).promise());
    }

    await Promise.all(promises);
    var endTime = performance.now();
    console.log("READ from " + bucketName + "/" + keyName, "in", endTime - startTime, "ms", workers, "workers", size, "size");
}
async function test() {
    for (let numWorkers of [1, 2, 4, 8, 16, 32, 64, 128]) {
        for (let size of [1024, 1024 * 1024, 1024 * 1024 * 1024]) {
                for (let i = 0; i < 10; i++) {
                let newSize = Math.round(size / numWorkers);
                let garbageBuffer = Buffer.alloc(newSize)

                var s3ParamsRead = {
                    Bucket:  bucketName,
                    Key: keyName
                };

                var s3ParamsWrite = {
                    Bucket:  bucketName,
                    Key: keyName,
                    Body: garbageBuffer
                };


                var startTime = performance.now();
                await testWrite(s3ParamsWrite, startTime, numWorkers, size)
                startTime = performance.now();
                await testRead(s3ParamsRead, startTime, numWorkers, size)
            }
        }
    }
}
test();