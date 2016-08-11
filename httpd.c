#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<strings.h>
#include<sys/stat.h>
#include<fcntl.h>
#define _SIZE_ 1024
#define TT printf("%s:%d",__func__,__LINE__)

static int startup(const char *_ip,const int _port)
{
	int sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("socket()");
		exit(1);
	}

	//不要TIME_WAIT
	int opt=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	struct sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_addr.s_addr=inet_addr(_ip);
	local.sin_port=htons(_port);

	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		perror("bind()");
		return 2;
	}

	if(listen(sock,5)<0)
	{
		perror("listen()");
		return 3;
	}
	return sock;
}


int get_line(int sock,char buf[],int len)
{
	if(!buf || len < 0)
	{
		return -1;
	}
	char c='\0';       //未初始化有bug！！！！
	int n=0;
	int i=0;

	//http请求是按行进行请求的
	while(i < len-1 && c != '\n')
	{
		n=recv(sock,&c,1,0);
		if(n>0)
		{
			if(c=='\r')
			{
				//进行窥探 并不读取
				n=recv(sock,&c,1,MSG_PEEK);
				if(n>0)
				{
					if(c=='\n')
					{
						recv(sock,&c,1,0);
					}
					else
					{
						c='\n';
					}
				}
			}
			buf[i++]=c;
		}
		else
			c='\n';
	}
	buf[i]='\0';
	return i;
}

void echo_error(int error)
{
	printf("is an error!\n");
}


static void clear_header(int sock)
{
	int ret=-1;
	char buf[_SIZE_];
	do
	{
		ret=get_line(sock,buf,sizeof(buf));
	}
	while(ret>0&&strcmp(buf,"\n")!=0);
}

void exec_cgi(int sock,const char *path,const char *method,const char *string)
{
	char buf[_SIZE_];
	int ret=-1;
	int content_length=-1;
	int cgi_input[2]={0,0};
	int cgi_output[2]={0,0};


	if(strcasecmp(method,"get")==0)
	{
		clear_header(sock);
	}
	else if(strcasecmp(method,"post")==0)
	{
		do
		{
			ret=get_line(sock,buf,sizeof(buf));
			if(ret > 0 && strncasecmp(buf,"content-length:",strlen("content-length:"))==0)
			{
				content_length=atoi(&buf[16]);
			}
		}
		while((ret>0) && (strcmp(buf,"\n")!=0));

		printf("length:%d\n",content_length);
		////////////////


		if(content_length==-1)
		{
			echo_error(sock);
			return;
		}
	}

	char res[_SIZE_]="HTTP/1.1 200 OK \r\n\r\n";
	send(sock,res,strlen(res),0);
	
	if(pipe(cgi_input)<0)
	{

	}
	if(pipe(cgi_output)<0)
	{

	}

	pid_t pid=-1;
	pid=fork();

	if(pid<0)
	{
		close(cgi_input[0]);
		close(cgi_input[1]);
		close(cgi_output[0]);
		close(cgi_output[1]);
	}
	else if(pid==0)
	{
		//child
		char method_env[_SIZE_];
		char query_string_env[_SIZE_];
		char length_env[_SIZE_];

		close(cgi_input[1]);
		close(cgi_output[0]);

		dup2(cgi_input[0],0);
		dup2(cgi_output[1],1);

		sprintf(method_env,"REQUEST_METHOD=%s",method);
		putenv(method_env);

		if(strcasecmp(method,"get")==0)
		{
			sprintf(query_string_env,"QUERY_STRING=%s",string);
			putenv(query_string_env);
		}
		else if(strcasecmp(method,"post")==0)
		{
			sprintf(length_env,"CONTENT_LENGTH=%d",content_length);
			putenv(length_env);
		}
		else
		{}

		///////////
		execl(path,path,NULL);

		exit(-1);
	}	
	else
	{
		//father
		close(cgi_input[0]);
		close(cgi_output[1]);

		size_t i=0;
		char c='\0';

		if(strcasecmp(method,"post")==0)
		{
			for(;i<content_length;++i)
			{
				recv(sock,&c,1,0);
				write(cgi_input[1],&c,1);
			}
		}

		while(read(cgi_output[0],&c,1)>0)
		{
			send(sock,&c,1,0);
		}

		close(cgi_input[1]);
		close(cgi_output[0]);

		waitpid(pid,NULL,0);
	}

}

void echo_www(int sock,const char *path,int size)
{
	int fd=open(path,O_RDONLY);
	if(fd<0)
	{
		///////
		TT;
		//////
		echo_error(sock);
		return;
	}
	//printf("server path:%s\n",path);
	char buf[_SIZE_];
	//http响应
	sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");

	send(sock,buf,strlen(buf),0);

	if(sendfile(sock,fd,NULL,size)<0)
	{
		/////
		TT;
		//////
		echo_error(sock);
		return;
	}
	close(fd);
}

void *request(void *arg)
{
	//自分离
	pthread_detach(pthread_self());
	int fd=(int)arg;

	char buf[_SIZE_];
	//printf("ali...\n");
	//四部分请求  读http请求
	int cgi = 0;  //默认get方法
	int ret=-1;
#ifdef _DEBUG1_
	do
	{
		ret=get_line(fd,buf,sizeof(buf));
		printf("%s",buf);
		fflush(stdout);
	}
	while(ret>0&&strcmp(buf,"\n")!=0);
#endif
//	GET / HTTP/1.1
//	Host: 127.0.0.1:8080
//	User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0
//	Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//	Accept-Language: en-US,en;q=0.5
//	Accept-Encoding: gzip, deflate
//	Connection: keep-alive

	//获取方法
	ret=get_line(fd,buf,sizeof(buf));
	if(ret<0)
	{
		///////
		TT;
		//////
		echo_error(fd);
		return (void *)1;
	}
	char method[_SIZE_];
	int i=0;  //buf index
	int j=0;  //method index
	while(!isspace(buf[i])&&i<sizeof(buf)&&j<sizeof(method)-1)
	{
		method[j++]=buf[i++];
	}
	method[j]='\0';
	
	//目前只处理get和post方法   strcasecmp忽略大小写的比较函数
	if(strcasecmp(method,"get")!=0&&strcasecmp(method,"post")!=0)
	{
		/////
		TT;
		/////
		echo_error(fd);
		//return (void *)2;
	}

	//post的处理  post的url在消息正文中
	if(strcasecmp(method,"post")==0)
	{
		cgi=1;
	}

	char url[_SIZE_];
	int k=0;
	//方法后面可能存在多个空格  所以要去空格
	while(isspace(buf[i]))
	{
		i++;
	}
    //获取url
	while(k<sizeof(url)&&i<sizeof(buf)&&!isspace(buf[i]))
	{
		url[k++]=buf[i++];
	}
	//get方法的处理  get后面是url
	char *string=url;
	if(strcasecmp(method,"get")==0)
	{
		while(*string!='?'&&*string!='\n')
		{
			string++;
		}
		if(*string=='?')
		{
			cgi=1;
			*string='\0';
			string++;
		}
	}

	char *path[_SIZE_];
	sprintf(path,"./htdoc%s",url);
	if(path[sizeof(path)-1]=='/')
	{
		strcat(path,"ali.html");
	}
#ifdef _DEBUG2_
	printf("method:%s\n",method);
	printf("url:%s\n",url);
	printf("path:%s\n",path);
#endif
	struct stat st;
	//获取目录属性
	if(stat(path,&st)<0)
	{
		/////
		TT;
		/////
		echo_error(fd);
	//	return (void*)3;
	}
	else
	{
		//DIR
		if(S_ISDIR(st.st_mode))
		{
			strcpy(path,"htdoc/ali.html");
		}
		//如果有可执行权限
		else if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
		{
			cgi=1;
		}
		else
		{}

		if(cgi)
		{
			exec_cgi(fd,path,method,string);
		}
		else
		{
			clear_header(fd);
			echo_www(fd,path,st.st_size);
		}
	}
	close(fd);
	return (void *)0;
}

static void Usage(const char *proc)
{
	printf("usage:%s [IP] [PORT]\n",proc);
}

int main(int argc,char *argv[])
{
	if(argc!=3)
	{
		Usage(argv[0]);
		return 4;
	}
	int listen_sock=startup(argv[1],atoi(argv[2]));
	
	while(1)
	{
		struct sockaddr_in peer;
		socklen_t len=sizeof(peer);

		int fd=accept(listen_sock,(struct sockaddr*)&peer,&len);

		if(fd>0)
		{
			printf("new client %s %d\n",inet_ntoa(peer.sin_addr),ntohs(peer.sin_port));
			pthread_t th;
			pthread_create(&th,NULL,request,(void*)fd);
			//pthread_detach(th);
		}
	}
	return 0;
}
