#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>
#include "iso9660.h"

#define IPV struct iso_prim_voldesc
#define SYS_PRE_SIZE 32768

void *to_void(void *p)
{
  return p;
}

void *move_to(void *p, int len)
{
  char *s = p;
  return s + len;
}

IPV *to_ipv(void *p)
{
  IPV *s = p;
  return s;
}

void print_help()
{
  printf("help\t: display command help\n");
  printf("info\t: display volume info\n");
  printf("ls\t: display directory content\n");
  printf("cd\t: change current directory\n");
  printf("tree\t: display the tree of the current directory\n");
  printf("get\t: copy file to local directory\n");
  printf("cat\t: display file content\n");
  printf("quit\t: program exit\n");
}

void print_info(IPV *ipv)
{
  printf("System Identifier: %.*s\n", ISO_SYSIDF_LEN, ipv->syidf);
  printf("Volume Identifier: %.*s\n", ISO_VOLIDF_LEN, ipv->vol_idf);
  printf("Block count: %d\n",ipv->vol_blk_count.le);
  printf("Block size: %d\n", ipv->vol_blk_size.le);
  printf("Creation date: %.*s\n", ISO_LDATE_LEN,ipv->date_creat);
  printf("Application Identifier: %.*s\n",ISO_APP_LEN, ipv->app_idf);
}

void iso_ls(char *f)
{
  int goto_next = 0;
  struct iso_dir *d_info = to_void(f);
  for (int i = 0; d_info->idf_len != 0; i++)
    {
      char is_dir = '-';
      char is_hidden = '-';
      uint32_t size = d_info->file_size.le;
      int year, month, day, hour, min;
      char *name;
      char f_name[100] = {0};
      if (i == 0)
	{
	  name = ".";
	  f_name[0] = '.';
	  f_name[1] = '\0';
	}
      else if (i == 1)
	{
	  name = "..";
	  f_name[0] = '.';
	  f_name[1] = '.';
	  f_name[2] = '\0';
	}
      else
	{
	  name = (char*)d_info + sizeof (struct iso_dir);
	  int j = 0;
	  int len = d_info->idf_len;
	  if (d_info->type != 2 && d_info->type != 3)
	    len -= 2;
	  for (; j < len; j++)
	    {
	      f_name[j] = name[j];
	    }
	}
      if (d_info->type == 2)
	is_dir = 'd';
      if (d_info->type == 1)
	is_hidden = 'h';
      if (d_info->type == 3)
	{
	  is_dir = 'd';
	  is_hidden = 'h';
	}
      char *date = d_info->date;

      year = 1900 + date[0];
      month = date[1];
      day = date[2];
      hour = date[3];
      min = date[4];

      printf("%c%c %9u %04d/%02d/%02d %02d:%02d %s\n",
	     is_dir, is_hidden, size, year, month, day, hour, min, f_name);
      goto_next = sizeof (struct iso_dir) + d_info->idf_len;
      if (d_info->idf_len % 2 == 0)
	goto_next++;
      d_info = move_to(d_info, goto_next);
    }
}

char *iso_cd(char *iso_file, char *cur_dir, char *cd_name,char **curdn,  char **pradn, char **predn, int *preb, int rootb)
{
  char dir_name[1024] = {0};
  int i = 0;
  for (; cd_name[i] != '\0'; i++)
    {
      if (cd_name[i] == ' ' && cd_name[i + 1] != ' ')
	break;
    }
  if (i != 3)
    i++;
  int j = 0;
  for (; cd_name[i + j] != '\0'; j++)
    {
      dir_name[j] = cd_name[i + j];
    }
  dir_name[j] = '\0';
  int goto_next = 0;
  struct iso_dir *d_info = to_void(cur_dir);

  if (strcmp(dir_name, ".") == 0)
    {
      printf("Changing to '%s' directory\n", *curdn);
      return cur_dir;
    }
  if (strcmp(dir_name, "..") == 0)
    {
      struct iso_dir *d_info = move_to(cur_dir, 34);
      char *to_dir = iso_file + d_info->data_blk.le * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
      printf("Changing to '%s' directory\n", *pradn);
      *predn = *curdn;
      *pradn = "root dir";
      *preb = d_info->data_blk.le;
      *curdn = *pradn;
      return to_dir;
    }
  if (strlen(dir_name) == 0)
    {
      char *to_dir = iso_file + rootb  * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
      printf("Changing to 'root dir' directory\n");
      *predn = *curdn;
      *preb = d_info->data_blk.le;
      *curdn = "root dir";
      return to_dir;
    }
  if (*dir_name =='-')
    {
      char *to_dir = iso_file + *preb  * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
      printf("Changing to '%s' directory\n", *predn);
      char *t = *predn;
      *preb = d_info->data_blk.le;
      *predn = *curdn;
      *curdn = t;
      return to_dir;
    }
  d_info = to_void(cur_dir);
  for (int i = 0; d_info->idf_len != 0; i++)
    {
      char *name = move_to(d_info, sizeof (struct iso_dir));
      int len = d_info->idf_len;
      if (d_info->type != 2)
	len -= 2;
      if (strncmp(name, dir_name, len) == 0)
        {
	  if (d_info->type != 2 && d_info->type != 3)
	    {
	      fprintf(stderr, "entry '%s' is not a directory\n", dir_name);
	      return cur_dir;
	    }
	  char *to_dir = iso_file + d_info->data_blk.le * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
	  printf("Changing to '%s' directory\n", dir_name);
	  *predn = *curdn;
	  struct iso_dir *c = to_void(cur_dir);
	  *preb = c->data_blk.le;
	  *pradn = *curdn;
	  *curdn = name;
	  return to_dir;
	}

      goto_next = sizeof (struct iso_dir) + d_info->idf_len;
      if (d_info->idf_len % 2 == 0)
	goto_next++;
      d_info = move_to(d_info, goto_next);
    }
  printf("unable to find '%s' directory entry\n", dir_name);
  return cur_dir;;
}

void iso_cat(char *iso_file, char *cur_dir,char *cat_name)
{
  char file_name[1024] = {0};
  int i = 0;
  for (; cat_name[i] != '\0'; i++)
    {
      if (cat_name[i] == ' ' && cat_name[i + 1] != ' ')
	break;
    }
  i++;
  int j = 0;
  for (; cat_name[i + j] != '\0'; j++)
    {
      file_name[j] = cat_name[i + j];
    }

  int goto_next = 0;
  struct iso_dir *d_info = move_to(cur_dir, 68);
  for (int i = 0; d_info->idf_len != 0; i++)
    {
      char *name = (char*)d_info + sizeof (struct iso_dir);
      int len = d_info->idf_len;
      if (d_info->type != 2)
	len -= 2;
      if (strncmp(name, file_name, len) == 0)
        {
	  if (d_info->type == 2 || d_info->type == 3)
	    {
	      fprintf(stderr, "entry '%s' is a directory\n", file_name);
	      return;
	    }
	  char *cat_file = iso_file + d_info->data_blk.le * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
	  write(1, cat_file, d_info->file_size.le);
	  return;
	}

      goto_next = sizeof (struct iso_dir) + d_info->idf_len;
      if (d_info->idf_len % 2 == 0)
	goto_next++;
      d_info = move_to(d_info, goto_next);
    }
  fprintf(stderr, "unable to find '%s' entry\n", file_name);
}

void iso_get(char *iso_file, char *cur_dir,char *get_name)
{
  char file_name[1024] = {0};
  int i = 0;
  for (; get_name[i] != '\0'; i++)
    {
      if (get_name[i] == ' ' && get_name[i + 1] != ' ')
	break;
    }
  i++;
  int j = 0;
  for (; get_name[i + j] != '\0'; j++)
    {
      file_name[j] = get_name[i + j];
    }

  int goto_next = 0;
  struct iso_dir *d_info = move_to(cur_dir, 68);
  for (int i = 0; d_info->idf_len != 0; i++)
    {
      char *name = (char*)d_info + sizeof (struct iso_dir);
      int len = d_info->idf_len;
      if (d_info->type != 2)
	len -= 2;
      if (strncmp(name, file_name, len) == 0)
        {
	  if (d_info->type == 2 || d_info->type == 3)
	    {
	      fprintf(stderr, "entry '%s' is a directory\n", file_name);
	      return;
	    }
	  char *get_file = iso_file + d_info->data_blk.le * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
	  int fd = open(file_name, O_CREAT | O_WRONLY, 0644);
	  write(fd, get_file, d_info->file_size.le);
	  close(fd);
	  return;
	}

      goto_next = sizeof (struct iso_dir) + d_info->idf_len;
      if (d_info->idf_len % 2 == 0)
	goto_next++;
      d_info = move_to(d_info, goto_next);
    }
  fprintf(stderr, "unable to find '%s' entry\n", file_name);
}

void iso_tree(int depth, char *cur_dir, int *nbdir, int *nbfile, int has_file[64])
{  
  if (depth == 0)
    {
      printf(".\n");
      depth++;
    }

  if (depth > 0)
    {
      int goto_next = 0;
      struct iso_dir *d_info = to_void(cur_dir + 68);
      for (int i = 0; d_info->idf_len != 0; i++)
	{
	  char *name;
	  char f_name[100] = {0};
	  name = (char*)d_info + sizeof (struct iso_dir);
	  int j = 0;
	  int len = d_info->idf_len;
	  if (d_info->type != 2 && d_info->type != 3)
	    len -= 2;
	  for (; j < len; j++)
	    {
	      f_name[j] = name[j];
	    }

	  for (int j = 0; j < depth - 1; j++)
	    {
	      if (has_file[j])
		printf("|");
	      else
		printf(" ");
	      printf("   ");
	    }

	  goto_next = sizeof (struct iso_dir) + d_info->idf_len;
	  if (d_info->idf_len % 2 == 0)
	    goto_next++;

	  struct iso_dir *temp = move_to(d_info, goto_next);
	  if (temp->idf_len != 0)
	    printf("|-- %s", f_name);
	  else
	    {
	      printf("+-- %s", f_name);
	      has_file[depth - 1] = 0;
	    }
	  if (d_info->type == 2 || d_info->type == 3)
	    {
	      if (temp->idf_len != 0)
		has_file[depth - 1] = 1;
	      (*nbdir)++;
	      printf("/\n");
	      struct iso_dir *c_info = to_void(cur_dir);
	      char *to_dir = cur_dir + (d_info->data_blk.le - c_info->data_blk.le) * ISO_BLOCK_SIZE;
	      iso_tree(depth + 1, to_dir, nbdir, nbfile, has_file);
	    }
	  else
	    {
	      (*nbfile)++;
	      printf("\n");
	    }
	  if (depth == 1 && temp->idf_len == 0)
	    {
	      printf("\n");
	      printf("%d directories, %d files\n", *nbdir, *nbfile);
	    }
	  d_info = move_to(d_info, goto_next);
	}
    }
}

int main(int argc, const char *argv[])
{
  if (argc != 2)
    {
      write(2, "usage: my_read_iso FILE\n", 40);
      return 1;
    }
  int f = open(argv[1], O_RDONLY);
  if (f == -1)
    {
      write(2, "usage: my_read_iso FILE\n", 40);
      return 1;
    }

  struct stat f_buf;
  fstat(f, &f_buf);
  char *iso_file = mmap(NULL, f_buf.st_size - SYS_PRE_SIZE,
			PROT_READ, MAP_PRIVATE, f, SYS_PRE_SIZE);

  struct iso_dir root_info = to_ipv(iso_file)->root_dir;
  char *root_dir = iso_file + root_info.data_blk.le * ISO_BLOCK_SIZE - SYS_PRE_SIZE;
  char *cur_dir = root_dir;

  char *cur_dir_name = "root dir";
  char *par_dir_name = "root dir";
  char *pre_dir_name = "root dir";
  int pre_dir_blk = root_info.data_blk.le;
  int root_blk = root_info.data_blk.le;

  char buf[1024] = {0};
  char cmd[1024] = {0};
  int p = 0;
  int flag = ! isatty(0);
  while (! isatty(0) || write(1, "> ", 2))
    {
      int len = read(0, buf, 1024);
      buf[len - 1] = '\0';

      int i = 0;
      for (; buf[p] != '\0' && buf[p] != '\n'; i++)
        {
          cmd[i] = buf[p];
          p++;
        }
      p++;
      cmd[i] = '\0';
      if (isatty(0))
        p = 0;

      if (strcmp(cmd, "help") == 0)
	print_help();
      else if (strncmp(cmd, "help ", 5) == 0)
	fprintf(stderr, "my_read_iso: help: command does not take an argument\n");
      else if (strcmp(cmd, "info") == 0)
	print_info(to_ipv(iso_file));
      else if (strncmp(cmd, "info ", 5) == 0)
	fprintf(stderr, "my_read_iso: info: command does not take an argument\n");
      else if (strcmp(cmd, "ls") == 0)
        iso_ls(cur_dir);
      else if (strncmp(cmd, "ls ", 3) == 0)
	fprintf(stderr, "my_read_iso: ls: command does not take an argument\n");
      else if (strncmp(cmd, "cd ", 3) == 0 || strncmp(cmd, "cd", 2) == 0)
	cur_dir = iso_cd(iso_file, cur_dir, cmd, &cur_dir_name,
			 &par_dir_name, &pre_dir_name, &pre_dir_blk, root_blk);
      else if (strncmp(cmd, "cat ", 4) == 0)
	iso_cat(iso_file, cur_dir, cmd);
      else if (strncmp(cmd, "get ", 4) == 0)
	iso_get(iso_file, cur_dir, cmd);
      else if (strcmp(cmd, "tree") == 0)
	{
	  int nbfile = 0;
	  int nbdir = 0;
	  int has_file[64] = {0};
	  iso_tree(0, cur_dir, &nbdir, &nbfile, has_file);
	  for (int k = 0; k < 64; k++)
	    has_file[k] = 0;
	}
      else if (strncmp(cmd, "tree ", 5) == 0)
	fprintf(stderr, "my_read_iso: tree: command does not take an argument\n");
      else if (strcmp(cmd, "quit") == 0)
	return 0;
      else if (! *cmd)
	{
	  if (flag)
	    return 0;
	  continue;
	}
      else
	fprintf(stderr, "my_read_iso: %s: unknown command\n", cmd);
      for (int k = 0; k < 1024; k++)
	cmd[k] = '\0';
    }
  return 0;
}
