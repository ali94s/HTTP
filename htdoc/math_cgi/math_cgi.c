#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void add(char *data_string)
{
	//data1=100&data2=200
    char *end = data_string + strlen(data_string);
	char *data[2] ;
	int i = 0;

	while(end != data_string)
	{
		if(*end == '=')
		{
			data[i++] = end + 1;
		}
		if(*end == '&')
		{
			*end = '\0';
		}
		--end;
	}

	printf("data1 : %s + data2 : %s = %d \n",data[0],data[1],atoi(data[0]) + atoi(data[1]));
}

int main()
{
	int content_length = -1;
	char method[1024];
	char query_string[1024];
	char put_data[1024];

	printf("<html>\n");
	printf("<head> MATH </head>\n");
	printf("<body>\n");

	strcpy(method,(char *)getenv("REQUEST_METHOD"));

	if(strcasecmp(method,"GET") == 0)
	{
		strcpy(query_string,(char *)getenv("QUERY_STRING"));
		add(query_string);
	}
	else if (strcasecmp(method,"POST") == 0)
	{
		content_length = atoi(getenv("CONTENT_LENGTH"));
		size_t i = 0;
		for ( ; i < content_length ; ++i)
		{
			read(0,&put_data[i], 1);
		}

		put_data[i] = '\0';
		add(put_data);
	}
//	printf("method=%s,query_string=%s,content_length=%d",method,query_string,content_length);	
	printf("</body>\n");
	printf("</html>\n");

	return 0;
}

