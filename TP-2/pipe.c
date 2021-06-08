#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <mqueue.h>

#define ULTIMOPEDIDO "-1"

int main(void)
{
   struct mq_attr attr;  
   attr.mq_flags = 0;  
   attr.mq_maxmsg = 10;
   attr.mq_msgsize = 4;
   attr.mq_curmsgs = 0;
   mqd_t mqdComandasEnc, mqdComandasCoc, mqdPedidosCoc, mqdPedidosDel;
   mqdComandasEnc = mq_open("/temp1",O_WRONLY | O_CREAT, 0777, &attr);
   if(mqdComandasEnc == -1)
      perror("mq_open_comEnc_failed()");
   else
      perror("mq_open_comEnc_ok()");
   mqdComandasCoc = mq_open("/temp1",O_RDONLY, 0777, &attr);
   mqdPedidosCoc  = mq_open("/temp2",O_WRONLY | O_CREAT, 0777, &attr);
   if(mqdPedidosCoc == -1)
      perror("mq_open_pedCoc_failed()");
   else
      perror("mq_open_pedCoc_ok()");
   mqdPedidosDel  = mq_open("/temp2",O_RDONLY, 0777, &attr);

   int status = mq_unlink("/temp1");
  if(status == -1)
    perror("unlink_cocDel_failed()");
  else
    perror("unlink_cocDel_ok()");

  status = mq_unlink("/temp2");
  if(status == -1)
    perror("unlink_delEnc_failed()");
  else
    perror("unlink_delEnc_ok()");
}
