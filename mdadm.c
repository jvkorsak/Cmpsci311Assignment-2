#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"
static int mounted=0;

int mdadm_mount(void) {
  // Complete your code here
  if (mounted){
    return -1;
  }
  uint32_t op=JBOD_MOUNT;
  jbod_operation(op, NULL);
  mounted=1;
  return 1;
}

int mdadm_unmount(void) {
  //Complete your code here
  if (mounted==0){
    return -1;
  }
  uint32_t op=JBOD_UNMOUNT;
  jbod_operation(op, NULL);
  mounted=0;
  return 1;
}

uint32_t prepare_jbod_op(uint32_t cmd,uint32_t disk_id,uint32_t block_id,uint32_t unused) {
    return (disk_id&0xff)|((block_id&0xff)<<4)|((cmd&0xff)<< 12)|((unused&0xfff)<<20);
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  //Complete your code here
  if (mounted!=1){
    return -3;
   }
  if (read_len>1024){
    return -2;
   }
  if ((start_addr+read_len)>(JBOD_NUM_DISKS+JBOD_DISK_SIZE)){
    return -1;
   }
  if (read_buf== NULL && read_len>0){
    return -4;
   } // All the cases where the function should not run and returns the value specified
  int curr_addr= start_addr;
  int bytes_left= 0;
  int bytes= 0;
  int curr_disk= curr_addr/JBOD_DISK_SIZE;
  uint8_t buf[JBOD_BLOCK_SIZE];
  while(read_len>bytes){
    int curr_block= (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
    int offset= curr_addr%JBOD_BLOCK_SIZE;
    uint32_t prepare_op=prepare_jbod_op(JBOD_SEEK_TO_DISK,curr_disk,0,0);
    jbod_operation(prepare_op,NULL); /// Find the disk we are in
    prepare_op=prepare_jbod_op(JBOD_SEEK_TO_BLOCK,curr_disk,curr_block,0);
    jbod_operation(prepare_op,NULL);//Find the block we are in
    prepare_op=prepare_jbod_op(JBOD_READ_BLOCK,0,0,0);
    jbod_operation(prepare_op,buf);//Read the block we found we are in
    bytes_left=read_len-bytes;
    if (offset+bytes_left<JBOD_BLOCK_SIZE){
      memcpy((read_buf+bytes),(buf+offset),bytes_left);
      bytes+=bytes_left;
      curr_addr+=bytes;
    }
    else{
      memcpy((read_buf+bytes),(buf+offset),(JBOD_BLOCK_SIZE-offset));
      bytes+=(JBOD_BLOCK_SIZE-offset);
      if (curr_block>255){
        curr_block=0;
        curr_disk+=1;
        prepare_op=prepare_jbod_op(JBOD_SEEK_TO_DISK,curr_disk,0,0);
        jbod_operation(prepare_op,NULL);
      }
      else{
      curr_addr+=bytes;
      }
    }
    offset=0;
  }
  return bytes;
  }

