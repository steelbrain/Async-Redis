<?hh
require('./AsyncRedis.hh');
class MyServerSidedApp{
  public AsyncRedis $RedisSub;
  public Redis $Redis;
  public function __construct(){
    $this->RedisSub = new AsyncRedis('tcp://localhost:6378');
    $this->RedisSub->subscribe('FirstChannel');
    $this->RedisSub->subscribe('SecondChannel');
    $this->RedisSub->OnMessage('FirstChannel', async function(string $Message){
      echo "First Channel $Message \n";
    });
    $this->RedisSub->OnMessage('SecondChannel', async function(string $Message){
      echo "Second Channel $Message \n";
    });

    $this->Redis = new Redis();
    $this->Redis->connect('localhost',6378);
  }
  public async function Init():Awaitable<void>{
    // This is where I do all the stuff, like reading from queues and responding to them etc.
    while(true){
      await SleepWaitHandle::create(1 * 1000000);
      $this->Redis->publish('FirstChannel', rand(0,100));
      $this->Redis->publish('SecondChannel', rand(0,100));
    }
  }
}
$App = new MyServerSidedApp();
GenArrayWaitHandle::create([$App->RedisSub->WaitHandle, $App->Init()])->join();
