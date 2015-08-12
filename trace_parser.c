#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> //getline()
#include <string.h> //strtok()
#include <stdbool.h>	//boolean
#include "common.h"
#include "trace_parser.h"

summary sum[SECTOR_ARRAY_SIZE] = {0};

void main(void)
{
    FILE *fpR1 = NULL;
    FILE *fpR2 = NULL;
    FILE *fpW = NULL;
    char *line1 = NULL;
    char *line2 = NULL;
    char *tmp = NULL;
    size_t len = 0;
    readLine string1;
    readLine string2;
    bool find = false;
    int i = 0;

    fpR1 = fopen(TRACE_INPUT_FILE_NAME, "r");
    fpR2 = fopen(TRACE_INPUT_FILE_NAME, "r");
    fpW = fopen(PARSER_OUTPUT_FILE_NAME, "w+");

    line1 = malloc(sizeof(char)*MAX_STR_LEN);
    line2 = malloc(sizeof(char)*MAX_STR_LEN);
    tmp = malloc(sizeof(char)*MAX_STR_LEN);

    while( getline(&line1, &len, fpR1) != -1 ){
	//Parsing
	parse_one_line(line1, &string1);
	
	//Only "D" action is eligible
	if( strcmp(string1.action,"D") == 0 ){
	    while( getline(&line2, &len, fpR2) != -1 ){
		//Parsing
		parse_one_line(line2, &string2);

		//Succeed to find
		if( string1.sSector == string2.sSector && \
			strcmp(string2.action,"C") == 0 ){
		    printf("Succeed\n");
		    printf("line1:%s", line1);
		    printf("line2:%s", line2);
		    find = true;
		    break;
		}
	    }//End of while()
	    //Fail to find
	    if(find == false){
		printf("Fail\n");
		printf("line1:%s", line1);
	    }
	    //Write back
	    line1[strlen(line1)-1] = '\0';
	    if(find == true){
		sprintf(tmp, "%s,%lf\n", line1, string2.sTime - string1.sTime);
		//Array update
		sum[string2.size/8].total += string2.sTime - string1.sTime;
		sum[string2.size/8].count ++;
	    }else{
		sprintf(tmp, "%s,%s\n", line1, "Error");
	    }
	    printf("%s\n", tmp);
	    fwrite(tmp, 1, strlen(tmp)+1, fpW);
	    //Re-initialize
	    find = false;
	    fseek(fpR2, 0, SEEK_SET);
	} //End of if()
    } //End of while()
    printf("\nThe END\n");
    
    //Print summary
    printf("sum = Total / Count\n");
    for(i=0; i<=SECTOR_ARRAY_SIZE; i++){
	if(sum[i].count != 0){ 
	    printf("%dKB = %lf/%d = %lf\n", i*8, sum[i].total, sum[i].count, sum[i].total/sum[i].count);
	}
    }
    fclose(fpR1);
    fclose(fpR2);
    fclose(fpW);
    free(line1);
    free(line2);
    free(tmp);
}

