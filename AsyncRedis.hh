<?hh //strict
class AsyncRedis{
  public ?WaitHandle<void> $WaitHandle;
  private int $Subscribers = 0;
  private resource $socket;
  public array<string, array<(function(string):Awaitable<void>)>> $Callbacks = [];
  public function __construct(string $server = 'localhost:6379'){
    $this->socket = stream_socket_client($server);
    stream_set_blocking($this->socket, 0);
  }
  public function subscribe(string $Channel):void{
    $ToWrite = "*2\r\n$9\r\nSUBSCRIBE\r\n$".strlen($Channel)."\r\n$Channel\r\n";
    fwrite($this->socket, $ToWrite);
    $this->__ParseResponse();

    if($this->WaitHandle === null){
      $this->WaitHandle = $this->__Init_Sub()->getWaitHandle();
    }
  }
  public function OnMessage(string $Channel, (function(string):Awaitable<void>) $Callback):void{
    ++ $this->Subscribers;
    $this->Callbacks[$Channel][] = $Callback;
  }
  public function unsubscribe(string $Channel):void{
    if(array_key_exists($Channel, $this->Callbacks) && count($this->Callbacks[$Channel]) !== 0){
      $this->Subscribers -= count($this->Callbacks[$Channel]);
      $ToWrite = "*2\r\n$11\r\nUNSUBSCRIBE\r\n$".strlen($Channel)."\r\n$Channel\r\n";
      fwrite($this->socket, $ToWrite);
      if($this->Subscribers === 0){
        fclose($this->socket);
      } else {
        $this->__ParseResponse();
      }
    }
  }
  private async function __Init_Sub():Awaitable<void>{
    while (is_resource($this->socket) && !feof($this->socket)) {
      $select = await stream_await($this->socket, STREAM_AWAIT_READ, 9999);
      switch ($select) {
        case STREAM_AWAIT_READY:
          if($this->Subscribers !== 0){
            $Response = $this->__ParseResponse();
            if(is_array($Response) && $Response[0] === 'message' && array_key_exists($Response[1], $this->Callbacks)){
              $WaitHandles = [];
              foreach($this->Callbacks[$Response[1]] as $Callback){
                $WaitHandles[] = $Callback($Response[2])->getWaitHandle();
              }
              await GenArrayWaitHandle::create($WaitHandles);
            }
          }
          break;
        case STREAM_AWAIT_CLOSED:
        return ;
        // Do nothing for timeouts
        default:
      }
    }
  }
  private function __ParseResponse():mixed{
    $line = fgets($this->socket);
    list($type, $result) = array($line[0], substr($line, 1, strlen($line) - 3));
    if ($type == '-') { // error message
      throw new Exception($result);
    } elseif ($type == '$') { // bulk reply
      if ($result == -1) {
        $result = '';
      } else {
        $line = fread($this->socket, $result + 2);
        $result = substr($line, 0, strlen($line) - 2);
      }
    } elseif ($type == '*') { // multi-bulk reply
      $count = ( int ) $result;
      for ($i = 0, $result = array(); $i < $count; $i++) {
        $result[] = $this->__ParseResponse();
      }
    }
    return $result;
  }
}
