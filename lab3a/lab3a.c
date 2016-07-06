#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define BUF_SIZE 1024

int num_block_groups;

struct super_block_t{
  unsigned int magic_num, num_inode, num_blocks, sz_block, sz_frag, block_per_group,
    inode_per_group, frag_per_group, db1;
  unsigned short inode_size;
};

struct group_block_t{
  unsigned int cont_block, free_block, free_inode, num_dir, inode_bitmap, block_bitmap,
    inode_table;
};

struct inode{
  int size;
  int* arr;
};

struct inode_complete{
  unsigned int inode, file_type, mode, owner, group, link_count, creation_time, modification_time,
    access_time, file_size, num_blocks, block_ptrs[15];
};

struct super_block_t* super;
struct group_block_t* group;
struct inode* inodes;
struct inode_complete* i_list;

void super_block(int fd){
  char buf[BUF_SIZE];
  
  int err = pread(fd, buf, BUF_SIZE, BUF_SIZE);
  if(err == -1){
    exit(9);
  }
  
  //get all of the numbers
  unsigned int* ptr = (unsigned int*)buf;
  super->num_inode = ptr[0];
  super->num_blocks = ptr[1];
  super->db1 = ptr[5];
  super->sz_block = 1024 << ptr[6];
  super->sz_frag = 1024 << ptr[7];
  super->block_per_group = ptr[8];
  super->frag_per_group = ptr[9];
  super->inode_per_group = ptr[10];
  super->magic_num = ptr[14];
  memcpy(&(super->inode_size), buf+88, 2);
  FILE* fd_out = fopen("super.csv", "w+");
  fprintf(fd_out, "%04x,%d,%d,%d,%d,%d,%d,%d,%d\n", super->magic_num, super->num_inode, 
super->num_blocks, super->sz_block, super->sz_frag, super->block_per_group, 
super->inode_per_group, super->frag_per_group, super->db1);
  fclose(fd_out);
}

void group_des(int fd){
  int block_size = super->sz_block;
  num_block_groups = ceil((double)super->num_blocks/(double)super->block_per_group);
  group = malloc(num_block_groups*sizeof(struct group_block_t));
  FILE* f = fopen("group.csv", "w+");
  char buf[BUF_SIZE];
  int i;
  for(i = 0; i < num_block_groups; i++){
    pread(fd, buf, BUF_SIZE, 2*block_size+32*i);
  
    int* arr = (int*)buf;
    group[i].block_bitmap = arr[0];
    group[i].inode_bitmap = arr[1];
    group[i].inode_table = arr[2];
    group[i].free_block = arr[3]&0xFFFF;
    group[i].free_inode = arr[3]>>16;
    group[i].num_dir = arr[4]&0xFFFF;
    
    int nblk= super->num_blocks;
    int bpg = super->block_per_group;
    group[i].cont_block = (nblk-bpg*i>bpg)?bpg:(nblk%bpg);
    fprintf(f, "%d,%d,%d,%d,%x,%x,%x\n", group[i].cont_block, group[i].free_block,
	    group[i].free_inode, group[i].num_dir, group[i].inode_bitmap,
	    group[i].block_bitmap, group[i].inode_table);
  }
  fclose(f);
}

void bitmap_entry(int fd){
  int i, j, start,k, block_size = super->sz_block, bpg = super->block_per_group,
    ipg = super->inode_per_group, arr_size = 2048, arr_idx = 0;
  
  inodes = (struct inode*)malloc(num_block_groups*sizeof(struct inode));
  char* buf=malloc(super->block_per_group+128);
  FILE* f = fopen("bitmap.csv", "w+");
  for(i = 0; i < num_block_groups; i++){
    arr_idx = 0;
    arr_size = super->inode_size;
    inodes[i].arr = (int*)malloc(arr_size*sizeof(int));
    start = (group[i].block_bitmap)*super->sz_block;
    pread(fd, buf, bpg/8+1, start);
    for(j = 0; j < bpg/8; j++){
      for(k = 0; k < ((j+1>bpg/8)?(bpg-8*j):8); k++){
	if(((1<<k)&((k>=8)?buf[j+1]:buf[j])) == 0){
	  fprintf(f, "%x,%d\n", group[i].block_bitmap, (8*j+k+bpg*i+1));
	}
      }
    }
   
    start = group[i].inode_bitmap*super->sz_block;
    pread(fd, buf, ipg/8+1, start);
    for(j = 0; j < ipg/8; j++){
      for(k = 0; k < ((j+1)>ipg/8?(ipg-8*j):8); k++){
	if(((1<<k)&((k>=8)?buf[j+1]:buf[j])) == 0){
	  fprintf(f,"%x,%d\n", group[i].inode_bitmap,i*super->inode_per_group+8*j+k+1);
        }
	else{
	  if(arr_idx >= arr_size){
	    arr_size *= 2;
	    inodes[i].arr = (int*)realloc(inodes[i].arr, arr_size*sizeof(int));
	  }
	  inodes[i].arr[arr_idx] = i*super->inode_per_group+8*j+k+1;
	  arr_idx++;
	}
      }
    }
    inodes[i].size = arr_idx;
  }
  fclose(f);
}

int inode_stuff(int fd){
  int num_inode = 0, i, j, k, l, offset, list_idx = 0, ipg = super->inode_per_group,
    bs = super->sz_block;
  unsigned int* tmp;
  FILE* f = fopen("inode.csv", "w+");
  char* buf = malloc(super->inode_size*sizeof(char));
  for(i = 0; i < num_block_groups; i++){
    num_inode += inodes[i].size;
  }
  i_list = (struct inode_complete*)malloc(num_inode*sizeof(struct inode_complete));
  for(i = 0; i < num_block_groups; i++){
    for(j = 0; j < inodes[i].size; j++){
      offset = group[i].inode_table*super->sz_block + (((inodes[i].arr[j] - 1)%ipg)*super->inode_size);
      pread(fd, buf, super->inode_size, offset);
      tmp = (int*)buf;
      i_list[list_idx].inode = inodes[i].arr[j];
      switch(tmp[0]&0xF000){
      case 0x8000:
      i_list[list_idx].file_type = 'f';
      break;
      case 0x4000:
      i_list[list_idx].file_type = 'd';
      break;
      case 0xA000:
      i_list[list_idx].file_type = 's';
      break;
      default:
      i_list[list_idx].file_type = '?';
      break;
      }

      i_list[list_idx].mode = tmp[0] & 0xFFFF;
      i_list[list_idx].owner = (tmp[0] & 0xFFFF0000)>>16; 
      i_list[list_idx].group = (tmp[6]&0xFFFF);
      i_list[list_idx].link_count = (tmp[6]&0xFFFF0000)>>16;
      i_list[list_idx].creation_time = tmp[3];
      i_list[list_idx].modification_time = tmp[4];
      i_list[list_idx].access_time = tmp[2];
      i_list[list_idx].file_size = tmp[1];
      unsigned int* ptr = (unsigned int*)(buf+28);
      i_list[list_idx].num_blocks = *ptr/(super->sz_block/512);//don't understand what's happening here
      fprintf(f, "%d,%c,%o,%d,%d,%d,%x,%x,%x,%d,%d",i_list[list_idx].inode, 
	      i_list[list_idx].file_type, i_list[list_idx].mode, (unsigned int)i_list[list_idx].owner,
	      i_list[list_idx].group,i_list[list_idx].link_count,
	      i_list[list_idx].creation_time, i_list[list_idx].modification_time,
	      i_list[list_idx].access_time,i_list[list_idx].file_size,
	      i_list[list_idx].num_blocks);
      for(l = 0; l < 15; l++){
	ptr = (unsigned int*)(buf+40);
	i_list[list_idx].block_ptrs[l] = ptr[l];
	fprintf(f, ",%x", ptr[l]);
      }
      fprintf(f, "\n");
      list_idx++;
    }
  }
  fclose(f);
  return list_idx;
}

void dir_parser(int fd, int size){
  int i, j, k, l, bs = super->sz_block, index, entry;
  unsigned int inode;
  unsigned short rec_len;
  unsigned char name_len, file_type;
  char* name;
  char buf[BUF_SIZE];
  FILE*f = fopen("directory.csv", "w+");
  
  struct inode_complete cur;
  for(i = 0; i < size; i++){
    cur = i_list[i];
    if(cur.file_type == 'd'){
      entry = 0;
      for(j = 0; j < 12; j++){
	index = 0;
	if(cur.num_blocks > j){
	  while(1){
	    pread(fd, buf, 263, bs*cur.block_ptrs[j]+index);
	    memcpy(&inode, buf, 4);
	    memcpy(&rec_len, buf+4, 2);
	    memcpy(&name_len, buf+6, 1);
	    memcpy(&file_type, buf+7, 1);
	    name = malloc(name_len*sizeof(char));
	    memcpy(name, buf+8, name_len);
	    if(inode != 0)
	      fprintf(f, "%d,%d,%d,%d,%d,\"%s\"\n", cur.inode, entry, rec_len, name_len, inode, name); 
	    index += rec_len;
	    entry++;
	    free(name);
	    if(index >= bs){
	      break;
	    }
	  }
	}
      }
      if(cur.num_blocks >= 12){
	if(cur.block_ptrs[12] != 0){//single indirect block
	  int blk_ptr;
	  for(j = 0; j < bs/4; j++){
	    pread(fd, &blk_ptr, 4, cur.block_ptrs[12]*bs+4*j);//gets you the pointer to the block
	    index = 0;
	    while(1){//go through the data pointed to by the indirect block;
	      pread(fd, buf, 263, bs*blk_ptr+index);
	      memcpy(&inode, buf, 4);
	      memcpy(&rec_len, buf+4, 2);
	      memcpy(&name_len, buf+6, 1);
	      memcpy(&file_type, buf+7, 1);
	      name = malloc(name_len*sizeof(char));
	      memcpy(name, buf+8, name_len);
	      if(inode != 0){
		fprintf(f, "%d,%d,%d,%d,%d,\"%s\"\n", cur.inode, entry, rec_len, name_len, inode, name);
	      }
	      index += rec_len;
	      entry++;
	      free(name);
	      if(index >= bs){
		break;
	      }
	    }
	  }
	}
	if(cur.block_ptrs[13] != 0){//double indirect
	  int blk_ptr1;
	  int blk_ptr2;
	  for(j = 0; j < bs/4; j++){
            pread(fd, &blk_ptr1, 4, cur.block_ptrs[13]*bs+4*j);//gets you the pointer to the block
	    for(k = 0; k < bs/4; k++){
	      pread(fd, &blk_ptr2, 4, blk_ptr1*bs+4*k);
	      index = 0;
	      while(1){
		pread(fd, buf, 263, bs*blk_ptr2+index);
		memcpy(&inode, buf, 4);
		memcpy(&rec_len, buf+4, 2);
		memcpy(&name_len, buf+6, 1);
		memcpy(&file_type, buf+7, 1);
		name = malloc(name_len*sizeof(char));
		memcpy(name, buf+8, name_len);
		if(inode != 0){
		  fprintf(f, "%d,%d,%d,%d,%d,\"%s\"\n", cur.inode, entry, rec_len, name_len, inode, name);
		}
		index += rec_len;
		entry++;
		free(name);
		if(index >= bs){
		  break;
		}
	      }
	    }
	  }
	}
      }
      if(cur.block_ptrs[14] != 0){//double indirect                                                                                                
	int blk_ptr1;
	int blk_ptr2;
	int blk_ptr3;
	for(j = 0; j < bs/4; j++){
	  pread(fd, &blk_ptr1, 4, cur.block_ptrs[14]*bs+4*j);//gets you the pointer to the block                                                      
	  for(k = 0; k < bs/4; k++){
	    pread(fd, &blk_ptr2, 4, blk_ptr1*bs+4*k);
	    for(l = 0; l < bs/4; l++){
	      pread(fd, &blk_ptr3, 4, blk_ptr2*bs+4*k);
	      index = 0;
	      while(1){
		pread(fd, buf, 263, bs*blk_ptr3+index);
		memcpy(&inode, buf, 4);
		memcpy(&rec_len, buf+4, 2);
		memcpy(&name_len, buf+6, 1);
		memcpy(&file_type, buf+7, 1);
		name = malloc(name_len*sizeof(char));
		memcpy(name, buf+8, name_len);
		if(inode != 0){
		  fprintf(f, "%d,%d,%d,%d,%d,\"%s\"\n", cur.inode, entry, rec_len, name_len, inode, name);
		}
		index += rec_len;
		entry++;
		free(name);
		if(index >= bs){
		  break;
		}
	      }
	    }
	  }
	}
      }
    }
  }
}

void indirect_block(int fd, int size){
  int i, j, k, l, bs = super->sz_block, index, entry;
  unsigned int inode;
  unsigned short rec_len;
  unsigned char name_len, file_type;
  char* name;
  char buf[BUF_SIZE];
  FILE*f = fopen("indirect.csv", "w+");
  
  struct inode_complete cur;
  for(i = 0; i < size; i++){
    cur = i_list[i];
    int blk_ptr;
    int blk_ptr1;
    int blk_ptr2;
    if(cur.num_blocks >= 13){
      //single indirect block
      for(j = 0; j < bs/4; j++){
	pread(fd, &blk_ptr, 4, cur.block_ptrs[12]*bs+4*j);
	if(blk_ptr != 0){
	  fprintf(f, "%x,%d,%x\n", cur.block_ptrs[12], j, blk_ptr);
	}
      }
    }
    //double indirect block
    if(cur.block_ptrs[13] != 0 && cur.num_blocks >= 14){
      for(j = 0; j < bs/4; j++){
	pread(fd, &blk_ptr, 4, cur.block_ptrs[13]*bs+4*j);
	if(blk_ptr != 0){
	  fprintf(f, "%x,%d,%x\n", cur.block_ptrs[13], j, blk_ptr);
	}
      }
      for(j = 0; j < bs/4; j++){
	pread(fd, &blk_ptr, 4, cur.block_ptrs[13]*bs+4*j);
	entry = 0;
	if(blk_ptr != 0){
	  for(k = 0; k < bs/4; k++){
	    pread(fd, &blk_ptr1, 4, blk_ptr*bs+4*k);
	    if(blk_ptr1 != 0)
	      fprintf(f, "%x,%d,%x\n", blk_ptr,entry, blk_ptr1);
	    entry++;
	  }
	}
      }
    }//end double indirect
    if(cur.block_ptrs[14] != 0 && cur.num_blocks == 15){
      for(j = 0; j < bs/4; j++){
        pread(fd, &blk_ptr, 4, cur.block_ptrs[14]*bs+4*j);
        if(blk_ptr != 0){
          fprintf(f, "%x,%d,%x\n", cur.block_ptrs[14], j, blk_ptr);
        }
      }
      for(j = 0; j < bs/4; j++){
        pread(fd, &blk_ptr, 4, cur.block_ptrs[14]*bs+4*j);
        entry = 0;
        if(blk_ptr != 0){
          for(k = 0; k < bs/4; k++){
            pread(fd, &blk_ptr1, 4, blk_ptr*bs+4*k);
            if(blk_ptr1 != 0)
              fprintf(f, "%x,%d,%x\n", blk_ptr,entry, blk_ptr1);
            entry++;
          }
        }
      }
      for(j = 0; j < bs/4; j++){
	pread(fd, &blk_ptr, 4, cur.block_ptrs[14]*bs+4*j);
	if(blk_ptr != 0){
	  for(k = 0; k < bs/4; k++){
	    pread(fd, &blk_ptr1, 4, blk_ptr*bs+4*k);
	    entry = 0;
	    if(blk_ptr != 0){
	      for(l = 0; l < bs/4; l++){
		pread(fd, &blk_ptr2, 4, blk_ptr1*bs+4*l);
		if(blk_ptr != 0){
		  fprintf(f, "%x,%d,%x\n", blk_ptr1, entry, blk_ptr2);
		}
		entry++;
	      }
	    }
	  }
	}
      }
    }
  }
}

int main(int argc, const char** argv){
  int i;
  
  int fd_di = open(argv[1], O_RDONLY);
  super = malloc(sizeof(struct super_block_t));
  super_block(fd_di);
  group_des(fd_di);
  bitmap_entry(fd_di);
  int size = inode_stuff(fd_di);
  dir_parser(fd_di, size);
  indirect_block(fd_di, size);
  close(fd_di);
}
