Async-Redis
==========
Async-Redis is a Redis [Pub/Sub][PubSub] Client for [HackLang][HackLang]. it makes use of HackLang's built-in [async functions][async].

### Usage

```hack
$RedisSub = new AsyncRedis('tcp://localhost:6379');
$RedisSub->subscribe('FirstChannel');
$RedisSub->subscribe('SecondChannel');
$RedisSub->OnMessage('FirstChannel', async function(string $Message){
  echo "First Channel $Message \n";
});
$RedisSub->OnMessage('SecondChannel', async function(string $Message){
  echo "Second Channel $Message \n";
});
$RedisSub->unsubscribe('FirstChannel');
```

### HowTo

Because Async-Redis uses HHVM's `WaitHandle`s, You'll have to join them to make sure the script doesn't kill the async function.
Here's how you can do it

```hack
$RedisSub = new AsyncRedis('tcp://localhost:6379');
$RedisSub->subscribe('FirstChannel');
$RedisSub->OnMessage('FirstChannel', async function(string $Message){
  echo "Message: $Message \n";
});
$RedisSub->WaitHandle->join(); // This will stop the program from halting, because this is in infinite loop, the program won't go any further
```
As you can see in the example above, You can not continue your program and listen for messages at the same time, It kind-of becomes a blocking way.
Therefore the solution I use is to make the init function of your Server Sided App an async one, so you can wait on both of them.
```hack
class MyServerSidedApp{
  public AsyncRedis $RedisSub;
  public function __construct(){
    $this->RedisSub = new AsyncRedis('tcp://localhost:6378');
    $this->RedisSub->subscribe('FirstChannel');
    $this->RedisSub->OnMessage('FirstChannel', async function(string $Message){
      echo "First Channel $Message \n";
    });
  }
  public async function Init():Awaitable<void>{
    // Do the regular app stuff here..
  }
}
$App = new MyServerSidedApp();
GenArrayWaitHandle::create([$App->RedisSub->WaitHandle, $App->Init()])->join();
// This way your program and the Redis subscriber will run co-currently
```
An extended version of this example can be found in `Example.hh`.

### License

This project is licensed under the terms of MIT License. See the LICENSE file for more info.

[PubSub]:http://redis.io/commands/pubsub
[HackLang]:http://hacklang.org
[async]:http://docs.hhvm.com/manual/en/hack.async.php