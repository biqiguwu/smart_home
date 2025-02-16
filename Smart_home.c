#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "cJSON.h"
#include "cJSON.c"
#include "head.h"

void* time_thread(void* arg) 
{
        
    lv_obj_t* time_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(time_label,&kai30,LV_STATE_DEFAULT);
    lv_obj_set_size(time_label,800,100);
    lv_obj_set_pos(time_label,350,0);
    while (1) 
    {
        // 获取当前的时间戳
        time_t currentTime = time(NULL);
        // 将时间戳转换为本地时间
        struct tm *localTime = localtime(&currentTime);
        // 使用 strftime 函数格式化时间字符串
        char timeString[100];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime); 
        // 绘制时间
        lv_label_set_text_fmt(time_label,"当前时间:%s",timeString);
        sleep(1);
    }

}
void clear_msgbox(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_current_target(e);
    if(lv_msgbox_get_active_btn(target) == 0)
    {
        lv_obj_add_flag(msgbox,LV_OBJ_FLAG_HIDDEN);
    }
}

void we_detail(lv_event_t* e)
{
    lv_obj_clear_flag(msgbox,LV_OBJ_FLAG_HIDDEN);
}


int weather_show(void)
{
     // 通过域名获取ip地址
    struct hostent* host = gethostbyname("api.seniverse.com");
    if(host == NULL)
    {
        perror("gethostbyname error");
        return -1;
    }

    // 获取ip地址
    struct in_addr ip_addr;
    memcpy(&ip_addr, host->h_addr_list[0], sizeof(struct in_addr));

    // 建立套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("socket error");
        return -1;
    }

    // 链接服务器
    struct sockaddr_in seraddr;
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(80);
    seraddr.sin_addr = ip_addr;

    int ret = connect(sockfd, (struct sockaddr*)&seraddr, sizeof(seraddr));
    if(ret < 0)
    {
        printf("connect error!\n");
    } 
    printf("connect successful\n");

    char* getheader = "GET /v3/weather/now.json?key=S-NEwIe1R4grjyry3&location=广州&language=zh-Hans&unit=c HTTP/1.1\r\nHost: api.seniverse.com\r\nConnection: keep-alive\r\n\r\n";
    usleep(10000);
    ret = write(sockfd,getheader,strlen(getheader));
    char readreply[1024] = {0};
    ret = read(sockfd, readreply, 1024);
    printf("%s\n",readreply);

    char* replyEnd = strstr(readreply,"\r\n\r\n");

    int len = 0;
    char* p = strstr(readreply,"Content-Length: ");
    if(p!=NULL)
    {
        sscanf(p,"Content-Length:%d",&len);
    }

    char weather[1024] = {0};
    strncpy(weather, replyEnd + 4, len);

    cJSON* root = cJSON_Parse(weather);
    if (root == NULL) 
    {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) 
        {
            printf("Error before: %s\n", error_ptr);
            return -1;
        }
        cJSON_Delete(root);
    }

    cJSON* array = cJSON_GetObjectItem(root, "results");
    //获取数组中的第一个值
    cJSON* arrayObj = cJSON_GetArrayItem(array, 0);
    cJSON* locateObj = cJSON_GetObjectItem(arrayObj, "location");
    cJSON* name = cJSON_GetObjectItem(locateObj, "name");      // 地点

    cJSON* nowObj = cJSON_GetObjectItem(arrayObj, "now");
    text = cJSON_GetObjectItem(nowObj, "text");         // 天气
    cJSON* temp = cJSON_GetObjectItem(nowObj, "temperature");  // 温度
    cJSON* feels_like = cJSON_GetObjectItem(nowObj, "feels_like");  // 体感温度
    cJSON* pressure = cJSON_GetObjectItem(nowObj, "pressure");  // 气压
    
    cJSON* humidity = cJSON_GetObjectItem(nowObj, "humidity"); // 湿度 
    cJSON* visibility = cJSON_GetObjectItem(nowObj, "visibility"); // 能见度
    cJSON* wind_direction = cJSON_GetObjectItem(nowObj, "wind_direction");  // 风向
    cJSON* update = cJSON_GetObjectItem(arrayObj, "last_update");      // 更新时间
    char time_part[19];
    strncpy(time_part, update->valuestring, 19);

    
    snprintf(formatted_text, sizeof(formatted_text), "%s 天气:%s 温度:%s\n湿度:%s\t能见度:%s\t更新时间:%s", 
    name->valuestring, text->valuestring, temp->valuestring,humidity->valuestring,visibility->valuestring,time_part);
    
    snprintf(weather_text, sizeof(weather_text), "地点:%s 天气:%s 温度:%s 湿度:%s\n能见度:%s 更新时间:%s", 
    name->valuestring, text->valuestring, temp->valuestring,humidity->valuestring,visibility->valuestring,time_part);
    
    snprintf(weather_all, sizeof(weather_all), "地点:%s\t天气:%s\t温度:%s\n湿度:%s\t体感温度:%s\t气压:%s\n能见度:%s\t风向:%s\n更新时间:%s", 
    name->valuestring, text->valuestring, temp->valuestring,humidity->valuestring,feels_like->valuestring,pressure->valuestring,visibility->valuestring,wind_direction->valuestring,time_part);
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label,formatted_text);
    lv_obj_set_size(label,800,100);
  

              lv_obj_t* tianqi = lv_img_create(obj1);
            if(strcmp(text->valuestring,"晴") == 0)     lv_img_set_src(tianqi,&sun);
            if(strcmp(text->valuestring,"多云") == 0)   lv_img_set_src(tianqi,&cloudy);
            if(strcmp(text->valuestring,"小雨") == 0)   lv_img_set_src(tianqi,&rain);
            
            lv_obj_set_pos(tianqi,-20,-30);
            lv_obj_set_size(tianqi,200,150);
  


    cJSON_Delete(root);

    close(sockfd);
    return 0;

}

void* get_weather(void* arg)
{
    int a = 0;
    while(1)
    {
        a = weather_show();
        if(a == 0)
        {

  

            lv_obj_t* bt_detail = lv_btn_create(obj1);
            lv_obj_set_size(bt_detail,150,40);
            lv_obj_t* label_detail = lv_label_create(bt_detail);
            lv_obj_set_style_text_font(bt_detail,&kai30,LV_STATE_DEFAULT);
            lv_label_set_text(label_detail,"详细信息");
            lv_obj_set_pos(bt_detail,20,120);
            lv_obj_add_event_cb(bt_detail, we_detail, LV_EVENT_CLICKED, NULL);  
            
            static const char* btns[] = {"返回", ""};

            msgbox = lv_msgbox_create(bg, "Detail",weather_all, btns, false);
            lv_obj_set_style_text_font(msgbox,&kai30,LV_STATE_DEFAULT);
            lv_obj_set_pos(msgbox, 100, 100);
            lv_obj_set_size(msgbox, 540, 250);
            lv_obj_add_flag(msgbox,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(msgbox,clear_msgbox,LV_EVENT_CLICKED,NULL);

            sleep(300);
        }
        if(a == -1)
        {
            sleep(2);
        }

    }
} 
void set_pos_size(lv_obj_t* obj, int w, int h)
{
    lv_obj_set_size(obj,250,200);
    lv_obj_set_pos (obj,w,h);
}

void event_fridge(lv_event_t* e)
{
    if(fridge_flag == 0)
    {
        fridge_flag = 1;
        lv_obj_clear_flag(fridge_up, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if(fridge_flag == 1)
    {
        fridge_flag = 0;
        lv_obj_add_flag(fridge_up, LV_OBJ_FLAG_HIDDEN);
    }
}

void event_tv(lv_event_t* e)
{
    if(tv_flag == 0)
    {
        tv_flag = 1;
        lv_obj_clear_flag(tv_up, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if(tv_flag == 1)
    {
        tv_flag = 0;
        lv_obj_add_flag(tv_up, LV_OBJ_FLAG_HIDDEN);
    }
}

void event_lamp1(lv_event_t* e)
{
    if(lamp_flag1 == 0)
    {
        lamp_flag1 = 1;
        lv_obj_clear_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag2 == 1)&&(lamp_flag3 == 1)&&(lamp_flag4 == 1)) 
        {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_ON, 7);
				
        return;
    }

    if(lamp_flag1 == 1)
    {
        lamp_flag1 = 0;
        lv_obj_add_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag2 == 0)&&(lamp_flag3 == 0)&&(lamp_flag4 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_OFF, 7);
    }
}

void event_lamp2(lv_event_t* e)
{
    if(lamp_flag2 == 0)
    {
        lamp_flag2 = 1;
        lv_obj_clear_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag1 == 1)&&(lamp_flag3 == 1)&&(lamp_flag4 == 1)) 
        {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_ON, 8);
        return;
    }

    if(lamp_flag2 == 1)
    {
        lamp_flag2 = 0;
        lv_obj_add_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag1 == 0)&&(lamp_flag3 == 0)&&(lamp_flag4 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_OFF, 8);
    }
}

void event_lamp3(lv_event_t* e)
{
    if(lamp_flag3 == 0)
    {
        lamp_flag3 = 1;
        lv_obj_clear_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag1 == 1)&&(lamp_flag2 == 1)&&(lamp_flag4 == 1)) {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_ON, 9);
        return;
    }

    if(lamp_flag3 == 1)
    {
        lamp_flag3 = 0;
        lv_obj_add_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag1 == 0)&&(lamp_flag2 == 0)&&(lamp_flag4 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_OFF, 9);
    }
}

void event_lamp4(lv_event_t* e)
{
    if(lamp_flag4 == 0)
    {
        lamp_flag4 = 1;
        lv_obj_clear_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag1 == 1)&&(lamp_flag2 == 1)&&(lamp_flag3 == 1)) {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_ON, 10);
        return;
    }

    if(lamp_flag4 == 1)
    {
        lamp_flag4 = 0;
        lv_obj_add_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
        if((lamp_flag1 == 0)&&(lamp_flag2 == 0)&&(lamp_flag3 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
        ioctl(led_fd, GEC6818_LED_OFF, 10);
    }
}


void event_lock(lv_event_t* e)
{
    if(lock_flag == 0)
    {
        lock_flag = 1;
        lv_obj_clear_flag(lock_up, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if(lock_flag == 1)
    {
        lock_flag = 0;
        lv_obj_add_flag(lock_up, LV_OBJ_FLAG_HIDDEN);
    }
}

void event_hvac(lv_event_t* e)
{
    if(hvac_flag == 0)
    {
        hvac_flag = 1;
        lv_obj_clear_flag(hvac_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(desi_sw, LV_STATE_DISABLED);
        return;
    }

    if(hvac_flag == 1)
    {
        hvac_flag = 0;
        lv_obj_add_flag(hvac_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
        lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
        lv_obj_add_state(desi_sw, LV_STATE_DISABLED);

    }
}

void event_hvac_up(lv_event_t* e)
{
    if(hvac_flag == 1)
    {
        hvac_temp++;
        lv_label_set_text_fmt(label_temp,"%d",hvac_temp);
    }   
}

void event_hvac_down(lv_event_t* e)
{
    if(hvac_flag == 1)
    {
        hvac_temp--;
        lv_label_set_text_fmt(label_temp,"%d",hvac_temp);
    }   
}

void event_cool(lv_event_t* e)
{
    if(hvac_flag == 0) {return;}
    if(cool_flag == 0)
    {
        cool_flag = 1;
        lv_obj_clear_flag(cool_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
        lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
        lv_obj_add_state(desi_sw, LV_STATE_DISABLED);

        return;
    }

    if(cool_flag == 1)
    {
        cool_flag = 0;
        lv_obj_add_flag(cool_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(desi_sw, LV_STATE_DISABLED);

    }
}

void event_hot(lv_event_t* e)
{
    if(hvac_flag == 0) {return;}
    if(hot_flag == 0)
    {
        hot_flag = 1;
        lv_obj_clear_flag(hot_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_add_state(desi_sw, LV_STATE_DISABLED);
        lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
        return;
    }

    if(hot_flag == 1)
    {
        hot_flag = 0;
        lv_obj_add_flag(hot_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(desi_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
    }
}

void event_airy(lv_event_t* e)
{
    if(hvac_flag == 0) {return;}
    if(airy_flag == 0)
    {
        airy_flag = 1;
        lv_obj_clear_flag(airy_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_add_state(desi_sw, LV_STATE_DISABLED);
        lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
        return;
    }

    if(airy_flag == 1)
    {
        airy_flag = 0;
        lv_obj_add_flag(airy_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(desi_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);

    }
}

void event_desi(lv_event_t* e)
{
    if(hvac_flag == 0) {return;}
    if(desi_flag == 0)
    {
        desi_flag = 1;
        lv_obj_clear_flag(desi_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
        lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
        return;
    }

    if(desi_flag == 1)
    {
        desi_flag = 0;
        lv_obj_add_flag(desi_up, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
        lv_obj_clear_state(hot_sw, LV_STATE_DISABLED); 
    }
}


void event_hvac_model(lv_event_t* e)
{
    lv_obj_clear_flag(hvac_bg,LV_OBJ_FLAG_HIDDEN);
}

void event_hvac_back(lv_event_t* e)
{
    lv_obj_add_flag(hvac_bg,LV_OBJ_FLAG_HIDDEN);
}

void event_all_up(lv_event_t* e)
{
    lamp_flag = 1;
    lamp_flag1 = 1;
    lamp_flag2 = 1;
    lamp_flag3 = 1;
    lamp_flag4 = 1;

    lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(lamp_sw1, LV_STATE_CHECKED);
    lv_obj_add_state(lamp_sw2, LV_STATE_CHECKED);
    lv_obj_add_state(lamp_sw3, LV_STATE_CHECKED);
    lv_obj_add_state(lamp_sw4, LV_STATE_CHECKED);

    ioctl(led_fd, GEC6818_LED_ON, 7);
    ioctl(led_fd, GEC6818_LED_ON, 8);
    ioctl(led_fd, GEC6818_LED_ON, 9);
    ioctl(led_fd, GEC6818_LED_ON, 10);


}

void event_all_down(lv_event_t* e)
{
    lamp_flag = 0;
    lamp_flag1 = 0;
    lamp_flag2 = 0;
    lamp_flag3 = 0;
    lamp_flag4 = 0;

    lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_state(lamp_sw1, LV_STATE_CHECKED);
    lv_obj_clear_state(lamp_sw2, LV_STATE_CHECKED);
    lv_obj_clear_state(lamp_sw3, LV_STATE_CHECKED);
    lv_obj_clear_state(lamp_sw4, LV_STATE_CHECKED);

    ioctl(led_fd, GEC6818_LED_OFF, 7);
    ioctl(led_fd, GEC6818_LED_OFF, 8);
    ioctl(led_fd, GEC6818_LED_OFF, 9);
    ioctl(led_fd, GEC6818_LED_OFF, 10);

}


void event_curtain_model(lv_event_t* e)
{
    lv_obj_clear_flag(cu_bg,LV_OBJ_FLAG_HIDDEN);
}

void event_curtain_back(lv_event_t* e)
{
    lv_obj_add_flag(cu_bg,LV_OBJ_FLAG_HIDDEN);
}


void event_lamp_model(lv_event_t* e)
{
    lv_obj_clear_flag(lp_bg, LV_OBJ_FLAG_HIDDEN);
}

void event_lamp_back(lv_event_t* e)
{
    lv_obj_add_flag(lp_bg, LV_OBJ_FLAG_HIDDEN);
}

void event_all_up_curtain(lv_event_t* e)
{
    curtain_flag = 1;
    curtain_flag1 = 1;
    curtain_flag2 = 1;
    curtain_flag3 = 1;
    curtain_flag4 = 1;

    lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_add_state(curtain_sw1, LV_STATE_CHECKED);
    lv_obj_add_state(curtain_sw2, LV_STATE_CHECKED);
    lv_obj_add_state(curtain_sw3, LV_STATE_CHECKED);
    lv_obj_add_state(curtain_sw4, LV_STATE_CHECKED);


}

void event_all_down_curtain(lv_event_t* e)
{
    curtain_flag = 0;
    curtain_flag1 = 0;
    curtain_flag2 = 0;
    curtain_flag3 = 0;
    curtain_flag4 = 0;
    
    lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_clear_state(curtain_sw1, LV_STATE_CHECKED);
    lv_obj_clear_state(curtain_sw2, LV_STATE_CHECKED);
    lv_obj_clear_state(curtain_sw3, LV_STATE_CHECKED);
    lv_obj_clear_state(curtain_sw4, LV_STATE_CHECKED);
}

void event_curtain1(lv_event_t* e)
{
    if(curtain_flag1 == 0)
    {
        curtain_flag1 = 1;
        lv_obj_clear_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag2 == 1)&&(curtain_flag3 == 1)&&(curtain_flag4 == 1)) 
        {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
        return;
    }

    if(curtain_flag1 == 1)
    {
        curtain_flag1 = 0;
        lv_obj_add_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag2 == 0)&&(curtain_flag3 == 0)&&(curtain_flag4 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
    }
}

void event_curtain2(lv_event_t* e)
{
    if(curtain_flag2 == 0)
    {
        curtain_flag2 = 1;
        lv_obj_clear_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag1 == 1)&&(curtain_flag3 == 1)&&(curtain_flag4 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
        return;
    }

    if(curtain_flag2 == 1)
    {
        curtain_flag2 = 0;
        lv_obj_add_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag1 == 0)&&(curtain_flag3 == 0)&&(curtain_flag4 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
    }
}

void event_curtain3(lv_event_t* e)
{
    if(curtain_flag3 == 0)
    {
        curtain_flag3 = 1;
        lv_obj_clear_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag2 == 1)&&(curtain_flag1 == 1)&&(curtain_flag4 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
        return;
    }

    if(curtain_flag3 == 1)
    {
        curtain_flag3 = 0;
        lv_obj_add_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag2 == 0)&&(curtain_flag1 == 0)&&(curtain_flag4 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
    }
}

void event_curtain4(lv_event_t* e)
{
    if(curtain_flag4 == 0)
    {
        curtain_flag4 = 1;
        lv_obj_clear_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag2 == 1)&&(curtain_flag3 == 1)&&(curtain_flag1 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
        return;
    }

    if(curtain_flag4 == 1)
    {
        curtain_flag4 = 0;
        lv_obj_add_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
        if((curtain_flag2 == 0)&&(curtain_flag3 == 0)&&(curtain_flag1 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
    }
}



void* remote_thread(void* arg)
{   
    int sockfd;
    char buf[BUFFER_SIZE];
    int temp = 0;
    int n;
    // 创建套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置待连接服务器的地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) 
    {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // 连接服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // 与服务器通信
     // 创建子进程
    pid_t pid = fork();

    if (pid == -1) 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    else if(pid == 0)
    {
        int i = 1;
        // zi进程用于发送数据
        while (1) 
        {
            if(i%360 == 0)
            {       
                if (send(sockfd,weather_text , 100,0) == -1) 
                {
                    perror("send");
                    exit(EXIT_FAILURE);
                }
            }
            i++;
            sleep(1);
        }
    }
    else  
    {
        // fu进程用于接收数据
        while (1) 
        {
            n = read(sockfd, buf, MAXLINE);
            
            printf("Received message from server:%s\n", buf);
            if(strcmp(buf,"weather") == 0)
            {
                 if (send(sockfd,weather_text ,100, 0) == -1) 
                {
                    perror("send");
                    exit(EXIT_FAILURE);
                }
            }
            if(strcmp(buf,"open fridge") == 0)
            {
                fridge_flag = 1;
                lv_obj_add_state(fridge_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(fridge_up, LV_OBJ_FLAG_HIDDEN); 
                
            }
            if(strcmp(buf,"close fridge") == 0)
            {
                fridge_flag = 0;
                lv_obj_clear_state(fridge_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(fridge_up, LV_OBJ_FLAG_HIDDEN);
            }
            if(strcmp(buf,"open tv") == 0)
            {
                tv_flag = 1;
                lv_obj_add_state(tv_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(tv_up, LV_OBJ_FLAG_HIDDEN); 
                
            }
            if(strcmp(buf,"close tv") == 0)
            {
                tv_flag = 0;
                lv_obj_clear_state(tv_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(tv_up, LV_OBJ_FLAG_HIDDEN);
            }
            if(strcmp(buf,"open lamp1") == 0)
            {
                lamp_flag1 = 1;
                lv_obj_add_state(lamp_sw1, LV_STATE_CHECKED);
                lv_obj_clear_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag2 == 1)&&(lamp_flag3 == 1)&&(lamp_flag4 == 1)) {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);} 
                ioctl(led_fd, GEC6818_LED_ON, 7);
            }
            if(strcmp(buf,"close lamp1") == 0)
            {
                lamp_flag1 = 0;
                lv_obj_clear_state(lamp_sw1, LV_STATE_CHECKED);
                lv_obj_add_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag2 == 0)&&(lamp_flag3 == 0)&&(lamp_flag4 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);} 
                ioctl(led_fd, GEC6818_LED_OFF, 7);
            }
            if(strcmp(buf,"open lamp2") == 0)
            {
                lamp_flag2 = 1;
                lv_obj_add_state(lamp_sw2, LV_STATE_CHECKED);
                lv_obj_clear_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag1 == 1)&&(lamp_flag3 == 1)&&(lamp_flag4 == 1)) {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
                ioctl(led_fd, GEC6818_LED_ON, 8);
                
            }

            if(strcmp(buf,"close lamp2") == 0)
            {
                lamp_flag2 = 0;
                lv_obj_clear_state(lamp_sw2, LV_STATE_CHECKED);
                lv_obj_add_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag1 == 0)&&(lamp_flag3 == 0)&&(lamp_flag4 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
                ioctl(led_fd, GEC6818_LED_OFF, 8);
            }
            
            if(strcmp(buf,"open lamp3") == 0)
            {
                lamp_flag3 = 1;
                lv_obj_add_state(lamp_sw3, LV_STATE_CHECKED);
                lv_obj_clear_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN); 
                if((lamp_flag1 == 1)&&(lamp_flag2 == 1)&&(lamp_flag4 == 1)) {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
                ioctl(led_fd, GEC6818_LED_ON, 9);
                
            }
            if(strcmp(buf,"close lamp3") == 0)
            {
                lamp_flag3 = 0;
                lv_obj_clear_state(lamp_sw3, LV_STATE_CHECKED);
                lv_obj_add_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag1 == 0)&&(lamp_flag2 == 0)&&(lamp_flag4 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
                ioctl(led_fd, GEC6818_LED_OFF, 9);
            }
            if(strcmp(buf,"open lamp4") == 0)
            {
                lamp_flag4 = 1;
                lv_obj_add_state(lamp_sw4, LV_STATE_CHECKED);
                lv_obj_clear_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag1 == 1)&&(lamp_flag2 == 1)&&(lamp_flag3 == 1)) {lamp_flag = 1;lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);} 
                ioctl(led_fd, GEC6818_LED_ON, 10);
            }
            if(strcmp(buf,"close lamp4") == 0)
            {
                lamp_flag4 = 0;
                lv_obj_clear_state(lamp_sw4, LV_STATE_CHECKED);
                lv_obj_add_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
                if((lamp_flag1 == 0)&&(lamp_flag2 == 0)&&(lamp_flag3 == 0)) {lamp_flag = 0;lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);}
                ioctl(led_fd, GEC6818_LED_OFF, 10);
            }
            

            if(strcmp(buf,"open lock") == 0)
            {
                lock_flag = 1;
                lv_obj_add_state(lock_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(lock_up, LV_OBJ_FLAG_HIDDEN); 
                
            }
            if(strcmp(buf,"close lock") == 0)
            {
                lock_flag = 0;
                lv_obj_clear_state(lock_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(lock_up, LV_OBJ_FLAG_HIDDEN);
            }
            if(strcmp(buf,"open curtain") == 0)
            {
                curtain_flag = 1;
                lv_obj_add_state(curtain_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN); 
                
            }
            if(strcmp(buf,"close curtain") == 0)
            {
                curtain_flag = 0;
                lv_obj_clear_state(curtain_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);
            }

            if(strcmp(buf,"open hvac") == 0)
            {
                hvac_flag = 1;
                lv_obj_add_state(hvac_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(hvac_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(desi_sw, LV_STATE_DISABLED);    
            }
            if(strcmp(buf,"close hvac") == 0)
            {
                hvac_flag = 0;
                lv_obj_clear_state(hvac_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(hvac_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
                lv_obj_add_state(desi_sw, LV_STATE_DISABLED);
            }
            
          
            if((hvac_flag == 1)&&(strncmp(buf,"temp",4) == 0))
            {   
                
                temp = (buf[5]-'0')*10+(buf[6]-'0');
                printf("temp = %d\n",temp);
                if(temp>32) 
                {memset(buf,0,sizeof(buf));continue;}
                if(buf[6] == 0) 
                {temp =buf[5]-'0';}
                
                hvac_temp = temp;
                lv_label_set_text_fmt(label_temp,"%d",hvac_temp);    
            }

            if((strcmp(buf,"open cool") == 0)&&(hvac_flag == 1)&&(hot_flag == 0)&&(airy_flag == 0)&&(desi_flag == 0))
            {
                cool_flag = 1;
                lv_obj_add_state(cool_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(cool_up, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
                lv_obj_add_state(desi_sw, LV_STATE_DISABLED); 
                
            }
            if((strcmp(buf,"close cool") == 0)&&(hvac_flag == 1))
            {
                cool_flag = 0;
                lv_obj_clear_state(cool_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(cool_up, LV_OBJ_FLAG_HIDDEN);
               
                lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(desi_sw, LV_STATE_DISABLED); 
            }

            if((strcmp(buf,"open hot") == 0)&&(hvac_flag == 1)&&(cool_flag == 0)&&(airy_flag == 0)&&(desi_flag == 0))
            {
                hot_flag = 1;
                lv_obj_add_state(hot_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(hot_up, LV_OBJ_FLAG_HIDDEN); 

                lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
                lv_obj_add_state(desi_sw, LV_STATE_DISABLED);
                
            }
            if((strcmp(buf,"close hot") == 0)&&(hvac_flag == 1))
            {
                hot_flag = 0;
                lv_obj_clear_state(hot_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(hot_up, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(airy_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(desi_sw, LV_STATE_DISABLED); 
            }

            if((strcmp(buf,"open airy") == 0)&&(hvac_flag == 1)&&(cool_flag == 0)&&(hot_flag == 0)&&(desi_flag == 0))
            {
                airy_flag = 1;
                lv_obj_add_state(airy_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(airy_up, LV_OBJ_FLAG_HIDDEN); 
                lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_add_state(desi_sw, LV_STATE_DISABLED);
                
            }
            if((strcmp(buf,"close airy") == 0)&&(hvac_flag == 1))
            {
                airy_flag = 0;
                lv_obj_clear_state(airy_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(airy_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(desi_sw, LV_STATE_DISABLED); 
            }
            if((strcmp(buf,"open desi") == 0)&&(hvac_flag == 1)&&(cool_flag == 0)&&(hot_flag == 0)&&(desi_flag) == 0)
            {
                desi_flag = 1;
                lv_obj_add_state(desi_sw, LV_STATE_CHECKED);
                lv_obj_clear_flag(desi_up, LV_OBJ_FLAG_HIDDEN); 
                lv_obj_add_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_add_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_add_state(airy_sw, LV_STATE_DISABLED);
                
            }
            if((strcmp(buf,"close desi") == 0)&&(hvac_flag == 1))
            {
                desi_flag = 0;
                lv_obj_clear_state(desi_sw, LV_STATE_CHECKED);
                lv_obj_add_flag(desi_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_state(cool_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(hot_sw, LV_STATE_DISABLED);
                lv_obj_clear_state(airy_sw, LV_STATE_DISABLED); 
            }

            if(strcmp(buf,"open all lamp") == 0)
            {
                lamp_flag = 1;
                lamp_flag1 = 1;
                lamp_flag2 = 1;
                lamp_flag3 = 1;
                lamp_flag4 = 1;
                
                lv_obj_clear_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_add_state(lamp_sw1, LV_STATE_CHECKED);
                lv_obj_add_state(lamp_sw2, LV_STATE_CHECKED);
                lv_obj_add_state(lamp_sw3, LV_STATE_CHECKED);
                lv_obj_add_state(lamp_sw4, LV_STATE_CHECKED);

                ioctl(led_fd, GEC6818_LED_ON, 7);
                ioctl(led_fd, GEC6818_LED_ON, 8);
                ioctl(led_fd, GEC6818_LED_ON, 9);
                ioctl(led_fd, GEC6818_LED_ON, 10);
                
            }
            if(strcmp(buf,"close all lamp") == 0)
            {
                lamp_flag = 0;
                lamp_flag1 = 0;
                lamp_flag2 = 0;
                lamp_flag3 = 0;
                lamp_flag4 = 0;

                lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_clear_state(lamp_sw1, LV_STATE_CHECKED);
                lv_obj_clear_state(lamp_sw2, LV_STATE_CHECKED);
                lv_obj_clear_state(lamp_sw3, LV_STATE_CHECKED);
                lv_obj_clear_state(lamp_sw4, LV_STATE_CHECKED);

                ioctl(led_fd, GEC6818_LED_OFF, 7);
                ioctl(led_fd, GEC6818_LED_OFF, 8);
                ioctl(led_fd, GEC6818_LED_OFF, 9);
                ioctl(led_fd, GEC6818_LED_OFF, 10);
            }

            if(strcmp(buf,"open all curtain") == 0)
            {
                curtain_flag = 1;
                curtain_flag1 = 1;
                curtain_flag2 = 1;
                curtain_flag3 = 1;
                curtain_flag4 = 1;
               
                lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
               
                lv_obj_add_state(curtain_sw1, LV_STATE_CHECKED);
                lv_obj_add_state(curtain_sw2, LV_STATE_CHECKED);
                lv_obj_add_state(curtain_sw3, LV_STATE_CHECKED);
                lv_obj_add_state(curtain_sw4, LV_STATE_CHECKED);
                
            }
            if(strcmp(buf,"close all curtain") == 0)
            {
                curtain_flag = 0;
                curtain_flag1 = 0;
                curtain_flag2 = 0;
                curtain_flag3 = 0;
                curtain_flag4 = 0;

                lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_clear_state(curtain_sw1, LV_STATE_CHECKED);
                lv_obj_clear_state(curtain_sw2, LV_STATE_CHECKED);
                lv_obj_clear_state(curtain_sw3, LV_STATE_CHECKED);
                lv_obj_clear_state(curtain_sw4, LV_STATE_CHECKED);
            }

            if(strcmp(buf,"open curtain1") == 0)
            {
                curtain_flag1 = 1;
                lv_obj_add_state(curtain_sw1, LV_STATE_CHECKED);
                lv_obj_clear_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
                if((curtain_flag2 == 1)&&(curtain_flag3 == 1)&&(curtain_flag4 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);} 
                
            }
            if(strcmp(buf,"close curtain1") == 0)
            {
                curtain_flag1 = 0;
                lv_obj_clear_state(curtain_sw1, LV_STATE_CHECKED);
                lv_obj_add_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
                if((curtain_flag2 == 0)&&(curtain_flag3 == 0)&&(curtain_flag4 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
            }
            if(strcmp(buf,"open curtain2") == 0)
            {
                curtain_flag2 = 1;
                lv_obj_add_state(curtain_sw2, LV_STATE_CHECKED);
                lv_obj_clear_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN); 
                if((curtain_flag1 == 1)&&(curtain_flag3 == 1)&&(curtain_flag4 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);} 
                
            }
            if(strcmp(buf,"close curtain2") == 0)
            {
                curtain_flag2 = 0;
                lv_obj_clear_state(curtain_sw2, LV_STATE_CHECKED);
                lv_obj_add_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
                if((curtain_flag1 == 0)&&(curtain_flag3 == 0)&&(curtain_flag4 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
            }
            
            
            if(strcmp(buf,"open curtain3") == 0)
            {
                curtain_flag3 = 1;
                lv_obj_add_state(curtain_sw3, LV_STATE_CHECKED);
                lv_obj_clear_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
                if((curtain_flag1 == 1)&&(curtain_flag2 == 1)&&(curtain_flag4 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}  
                
            }
            if(strcmp(buf,"close curtain3") == 0)
            {
                curtain_flag3 = 0;
                lv_obj_clear_state(curtain_sw3, LV_STATE_CHECKED);
                lv_obj_add_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
                if((curtain_flag1 == 0)&&(curtain_flag2 == 0)&&(curtain_flag4 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}

            }
            if(strcmp(buf,"open curtain4") == 0)
            {
                curtain_flag4 = 1;
                lv_obj_add_state(curtain_sw4, LV_STATE_CHECKED);
                lv_obj_clear_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
                if((curtain_flag1 == 1)&&(curtain_flag2 == 1)&&(curtain_flag3 == 1)) {curtain_flag = 1;lv_obj_clear_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}   
                
            }
            if(strcmp(buf,"close curtain4") == 0)
            {
                curtain_flag4 = 0;
                lv_obj_clear_state(curtain_sw4, LV_STATE_CHECKED);
                lv_obj_add_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN); 
                if((curtain_flag1 == 0)&&(curtain_flag2 == 0)&&(curtain_flag3 == 0)) {curtain_flag = 0;lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);}
            }  
            if(strcmp(buf,"open alarm") == 0)
            {
                condi = 0;
                lv_obj_clear_flag(alarm_up,LV_OBJ_FLAG_HIDDEN);
                while(condi == 0)
                {
                    rt = ioctl(beep_fd, GEC6818_BEEP_PWM_ON, duty);
                }
            }
            if(strcmp(buf,"close alarm") == 0)
            {
                lv_obj_add_flag(alarm_up,LV_OBJ_FLAG_HIDDEN);
                rt = ioctl(beep_fd, GEC6818_BEEP_PWM_OFF, duty);
            }

            memset(buf,0,sizeof(buf));
        }
    }
}   


        
void bt_event_cb(lv_event_t* e)
{
    
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSING) 
    {
        lv_obj_clear_flag(bt_pr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(alarm_up, LV_OBJ_FLAG_HIDDEN);
        rt = ioctl(beep_fd, GEC6818_BEEP_PWM_ON, duty);
           
    }
    else if (code == LV_EVENT_RELEASED) 
    {
        // 处理图片松开事件
        lv_obj_add_flag(bt_pr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(alarm_up, LV_OBJ_FLAG_HIDDEN);
        rt = ioctl(beep_fd, GEC6818_BEEP_PWM_OFF, duty);
    }
}


void UI_Start(void)
{
    lv_obj_t* wt = lv_obj_create(lv_scr_act());

    lv_obj_set_size(wt,800,480);
    lv_obj_set_pos(wt,0,0);
    bg = lv_img_create(wt);
    lv_img_set_src(bg,&background);
    lv_obj_set_size(bg,800,480);
    lv_obj_set_pos(bg,-20,-20);
    
    lv_obj_t* obj2 = lv_obj_create(bg);
    lv_obj_t* obj3 = lv_obj_create(bg);
    lv_obj_t* obj4 = lv_obj_create(bg);
    lv_obj_t* obj5 = lv_obj_create(bg);
    lv_obj_t* obj6 = lv_obj_create(bg);
    
    obj1 = lv_obj_create(bg);
    lv_obj_set_size(obj1,250,200);
    lv_obj_set_pos(obj1,0,60);

    set_pos_size(obj2,280,60);
    set_pos_size(obj3,550,60);
    set_pos_size(obj4,0,270);
    set_pos_size(obj5,280,270);
    set_pos_size(obj6,550,270);

 

    //fridge
    lv_obj_t* fridge_down = lv_img_create(obj4);
    lv_img_set_src(fridge_down,&fridge);
    lv_obj_set_style_img_recolor(fridge_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(fridge_down,200,LV_PART_MAIN);
    lv_obj_set_pos(fridge_down,-10,-20);
    lv_obj_set_size(fridge_down,100,100);
    
    fridge_up = lv_img_create(obj4);
    lv_img_set_src(fridge_up,&fridge);
    lv_obj_set_pos(fridge_up,-10,-20);
    lv_obj_set_size(fridge_up,100,100);
    lv_obj_add_flag(fridge_up, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_fridge = lv_label_create(obj4);
    lv_obj_set_style_text_font(label_fridge,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_fridge,"冰箱");
    lv_obj_set_size(label_fridge,60,50);
    lv_obj_align_to(label_fridge,fridge_down,LV_ALIGN_OUT_BOTTOM_MID,0,-10);

    fridge_sw = lv_switch_create(obj4);
    lv_obj_set_size(fridge_sw,70,30);
    lv_obj_align_to(fridge_sw,label_fridge,LV_ALIGN_OUT_BOTTOM_MID,0,-10);
    lv_obj_add_event_cb(fridge_sw,event_fridge,LV_EVENT_CLICKED,NULL);

    //tv
    lv_obj_t* tv_down = lv_img_create(obj4);
    lv_img_set_src(tv_down,&tv);
    lv_obj_set_style_img_recolor(tv_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(tv_down,200,LV_PART_MAIN);
    lv_obj_set_pos(tv_down,90,-20);
    lv_obj_set_size(tv_down,100,100);
    
    tv_up = lv_img_create(obj4);
    lv_img_set_src(tv_up,&tv);
    lv_obj_set_pos(tv_up, 90,-20);
    lv_obj_set_size(tv_up,100,100);
    lv_obj_add_flag(tv_up, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_tv = lv_label_create(obj4);
    lv_obj_set_style_text_font(label_tv,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_tv,"电视");
    lv_obj_set_size(label_tv,60,50);
    lv_obj_align_to(label_tv,tv_down,LV_ALIGN_OUT_BOTTOM_MID,0,-10);
    
    tv_sw = lv_switch_create(obj4);
    lv_obj_set_size(tv_sw,70,30);
    lv_obj_align_to(tv_sw,label_tv,LV_ALIGN_OUT_BOTTOM_MID,0,-10);
    lv_obj_add_event_cb(tv_sw,event_tv,LV_EVENT_CLICKED,NULL);

    //门锁
    lv_obj_t* lock_down = lv_img_create(obj2);
    lv_img_set_src(lock_down,&lock);
    lv_obj_set_style_img_recolor(lock_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(lock_down,200,LV_PART_MAIN);
    lv_obj_set_pos(lock_down,0,-20);
    lv_obj_set_size(lock_down,100,100);
    
    lock_up = lv_img_create(obj2);
    lv_img_set_src(lock_up,&lock);
    lv_obj_set_pos(lock_up, 0,-20);
    lv_obj_set_size(lock_up,100,100);
    lv_obj_add_flag(lock_up, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_lock = lv_label_create(obj2);
    lv_obj_set_style_text_font(label_lock,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lock,"门锁");
    lv_obj_set_size(label_lock,60,50);
    lv_obj_align_to(label_lock,lock_down,LV_ALIGN_OUT_BOTTOM_MID,-10,0);

    lock_sw = lv_switch_create(obj2);
    lv_obj_set_size(lock_sw,70,30);
    lv_obj_align_to(lock_sw,label_lock,LV_ALIGN_OUT_BOTTOM_MID,0,-10);
    lv_obj_add_event_cb(lock_sw,event_lock,LV_EVENT_CLICKED,NULL);
    
    //警报器
    lv_obj_t* alarm_down = lv_img_create(obj2);
    lv_img_set_src(alarm_down,&alarm_beep);
    lv_obj_set_style_img_recolor(alarm_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(alarm_down,200,LV_PART_MAIN);
    lv_obj_set_pos(alarm_down,100,-20);
    lv_obj_set_size(alarm_down,100,100);
    
    alarm_up = lv_img_create(obj2);
    lv_img_set_src(alarm_up,&alarm_beep);
    lv_obj_set_pos(alarm_up, 100,-20);
    lv_obj_set_size(alarm_up,100,100);
    lv_obj_add_flag(alarm_up, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* label_alarm = lv_label_create(obj2);
    lv_obj_set_style_text_font(label_alarm,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_alarm,"警报器");
    lv_obj_set_size(label_alarm,90,50);
    lv_obj_align_to(label_alarm,alarm_down,LV_ALIGN_OUT_BOTTOM_MID,-10,0);

    lv_obj_t * bt_re = lv_imgbtn_create(obj2);
    lv_img_set_src(bt_re,&bt_release);
    lv_obj_set_pos(bt_re,120,110);
    lv_obj_set_size(bt_re,50,50);

    bt_pr = lv_img_create(obj2);
    lv_img_set_src(bt_pr,&bt_press);
    lv_obj_set_pos(bt_pr,120,110);
    lv_obj_set_size(bt_pr,50,50);
    lv_obj_add_flag(bt_pr, LV_OBJ_FLAG_HIDDEN);

    // 设置按下事件回调函数
    lv_obj_add_event_cb(bt_re,bt_event_cb,LV_EVENT_ALL,NULL);
   


   
    //窗帘
    lv_obj_t* curtain_down = lv_img_create(obj6);
    lv_img_set_src(curtain_down,&curtain);
    lv_obj_set_style_img_recolor(curtain_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(curtain_down,200,LV_PART_MAIN);
    lv_obj_set_pos(curtain_down,-20,-20);
    lv_obj_set_size(curtain_down,100,150);
    
    curtain_up = lv_img_create(obj6);
    lv_img_set_src(curtain_up,&curtain);
    lv_obj_set_pos(curtain_up, -20,-20);
    lv_obj_set_size(curtain_up,100,150);
    lv_obj_add_flag(curtain_up, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_curtain = lv_label_create(obj6);
    lv_obj_set_style_text_font(label_curtain,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_curtain,"窗帘");
    lv_obj_set_size(label_curtain,60,50);
    lv_obj_align_to(label_curtain,curtain_down,LV_ALIGN_OUT_BOTTOM_MID,0,0);

    //窗帘按钮
    lv_obj_t* curtain_model = lv_btn_create(obj6);
    lv_obj_set_size(curtain_model,100,50);
    lv_obj_align_to(curtain_model,curtain_down,LV_ALIGN_OUT_RIGHT_MID,10,-40);
    lv_obj_t* label_model_curtain = lv_label_create(curtain_model);
    lv_obj_set_style_text_font(curtain_model,&kai30,LV_STATE_DEFAULT);

    lv_label_set_text(label_model_curtain,"设置");
    lv_obj_add_event_cb(curtain_model,event_curtain_model,LV_EVENT_CLICKED,NULL); //设置按钮事件
 
    lv_obj_t* all_up_curtain = lv_btn_create(obj6);
    lv_obj_set_size(all_up_curtain,100,50);
    lv_obj_t* label_up_curtain = lv_label_create(all_up_curtain);
    lv_obj_set_style_text_font(all_up_curtain,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_up_curtain,"全开");
    lv_obj_align_to(all_up_curtain,curtain_down,LV_ALIGN_OUT_RIGHT_MID,10,20);
    lv_obj_add_event_cb(all_up_curtain,event_all_up_curtain,LV_EVENT_CLICKED,NULL);

    lv_obj_t* all_down_curtain = lv_btn_create(obj6);
    lv_obj_set_size(all_down_curtain,100,50);
    lv_obj_t* label_down_curtain = lv_label_create(all_down_curtain);
    lv_obj_set_style_text_font(all_down_curtain,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_down_curtain,"全关");
    lv_obj_align_to(all_down_curtain,all_up_curtain,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    lv_obj_add_event_cb(all_down_curtain,event_all_down_curtain,LV_EVENT_CLICKED,NULL);

    //窗帘二级界面
    cu_bg = lv_img_create(bg);
    lv_img_set_src(cu_bg,&cur_bg);
    lv_obj_set_size(cu_bg,800,480);
    lv_obj_set_pos(cu_bg,0,0);
    lv_obj_add_flag(cu_bg, LV_OBJ_FLAG_HIDDEN);

    //退出图标事件
    lv_obj_t* cu_back = lv_btn_create(cu_bg);
    lv_obj_set_size(cu_back,100,50);
    lv_obj_t* label_curtain_back = lv_label_create(cu_back);
    lv_obj_set_style_text_font(cu_back,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_curtain_back,"返回");
    lv_obj_set_pos(cu_back,350,400);
    lv_obj_add_event_cb(cu_back,event_curtain_back,LV_EVENT_CLICKED,NULL);

    //curtain二级界面
    lv_obj_t* curtain_down1 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_down1,&curtains);
    lv_obj_set_style_img_recolor(curtain_down1,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(curtain_down1,200,LV_PART_MAIN);
    lv_obj_set_pos(curtain_down1,25,50);
    lv_obj_set_size(curtain_down1,150,200);
  
    curtain_up1 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_up1,&curtains);
    lv_obj_set_pos(curtain_up1, 25,50);
    lv_obj_set_size(curtain_up1,150,200);
    lv_obj_add_flag(curtain_up1, LV_OBJ_FLAG_HIDDEN);
      
    lv_obj_t* label_cu1 = lv_label_create(cu_bg);
    lv_obj_set_style_text_font(label_cu1,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_cu1,"卧室");
    lv_obj_set_size(label_cu1,60,50);
    lv_obj_align_to(label_cu1,curtain_down1,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    
    curtain_sw1 = lv_switch_create(cu_bg);
    lv_obj_set_size(curtain_sw1,100,50);
    lv_obj_align_to(curtain_sw1,label_cu1,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(curtain_sw1,event_curtain1,LV_EVENT_CLICKED,NULL);


    lv_obj_t* curtain_down2 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_down2,&curtains);
    lv_obj_set_style_img_recolor(curtain_down2,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(curtain_down2,200,LV_PART_MAIN);
    lv_obj_set_pos(curtain_down2,225,50);
    lv_obj_set_size(curtain_down2,150,200);
     
    curtain_up2 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_up2,&curtains);
    lv_obj_set_pos(curtain_up2, 225,50);
    lv_obj_set_size(curtain_up2,150,200);
    lv_obj_add_flag(curtain_up2, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_cu2 = lv_label_create(cu_bg);
    lv_obj_set_style_text_font(label_cu2,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_cu2,"客厅");
    lv_obj_set_size(label_cu2,60,50);
    lv_obj_align_to(label_cu2,curtain_down2,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    
    curtain_sw2 = lv_switch_create(cu_bg);
    lv_obj_set_size(curtain_sw2,100,50);
    lv_obj_align_to(curtain_sw2,label_cu2,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(curtain_sw2,event_curtain2,LV_EVENT_CLICKED,NULL);


    lv_obj_t* curtain_down3 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_down3,&curtains);
    lv_obj_set_style_img_recolor(curtain_down3,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(curtain_down3,200,LV_PART_MAIN);
    lv_obj_set_pos(curtain_down3,425,50);
    lv_obj_set_size(curtain_down3,150,200);
    
    curtain_up3 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_up3,&curtains);
    lv_obj_set_pos(curtain_up3, 425,50);
    lv_obj_set_size(curtain_up3,150,200);
    lv_obj_add_flag(curtain_up3, LV_OBJ_FLAG_HIDDEN);
     
    lv_obj_t* label_cu3 = lv_label_create(cu_bg);
    lv_obj_set_style_text_font(label_cu3,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_cu3,"厨房");
    lv_obj_set_size(label_cu3,60,50);
    lv_obj_align_to(label_cu3,curtain_down3,LV_ALIGN_OUT_BOTTOM_MID,0,10);
  
    curtain_sw3 = lv_switch_create(cu_bg);
    lv_obj_set_size(curtain_sw3,100,50);
    lv_obj_align_to(curtain_sw3,label_cu3,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(curtain_sw3,event_curtain3,LV_EVENT_CLICKED,NULL);

    lv_obj_t* curtain_down4 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_down4,&curtains);
    lv_obj_set_style_img_recolor(curtain_down4,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(curtain_down4,200,LV_PART_MAIN);
    lv_obj_set_pos(curtain_down4,625,50);
    lv_obj_set_size(curtain_down4,150,200);

    curtain_up4 = lv_img_create(cu_bg);
    lv_img_set_src(curtain_up4,&curtains);
    lv_obj_set_pos(curtain_up4, 625,50);
    lv_obj_set_size(curtain_up4,150,200);
    lv_obj_add_flag(curtain_up4, LV_OBJ_FLAG_HIDDEN);
     
    lv_obj_t* label_cu4 = lv_label_create(cu_bg);
    lv_obj_set_style_text_font(label_cu4,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_cu4,"书房");
    lv_obj_set_size(label_cu4,60,50);
    lv_obj_align_to(label_cu4,curtain_down4,LV_ALIGN_OUT_BOTTOM_MID,0,0);
  
    curtain_sw4 = lv_switch_create(cu_bg);
    lv_obj_set_size(curtain_sw4,100,50);
    lv_obj_align_to(curtain_sw4,label_cu4,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(curtain_sw4,event_curtain4,LV_EVENT_CLICKED,NULL);



    //空调  一个部件一百多行
    lv_obj_t* hvac_down = lv_img_create(obj5);
    lv_img_set_src(hvac_down,&hvac);
    lv_obj_set_style_img_recolor(hvac_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(hvac_down,200,LV_PART_MAIN);
    lv_obj_set_pos(hvac_down,0,-30);
    lv_obj_set_size(hvac_down,200,100);
    
    hvac_up = lv_img_create(obj5);
    lv_img_set_src(hvac_up,&hvac);
    lv_obj_set_pos(hvac_up, 0,-30);
    lv_obj_set_size(hvac_up,200,100);
    lv_obj_add_flag(hvac_up, LV_OBJ_FLAG_HIDDEN);
    
    //模式
    lv_obj_t* hvac_model = lv_btn_create(obj5);
    lv_obj_set_size(hvac_model,50,50);
    lv_obj_align_to(hvac_model,hvac_down,LV_ALIGN_OUT_BOTTOM_MID,-60,-10);
    lv_obj_set_style_bg_img_src(hvac_model,LV_SYMBOL_SETTINGS,LV_PART_MAIN);
    lv_obj_add_event_cb(hvac_model,event_hvac_model,LV_EVENT_CLICKED,NULL); //设置按钮事件
    
    //开关
    hvac_sw = lv_switch_create(obj5);
    lv_obj_set_size(hvac_sw,80,40);
    lv_obj_align_to(hvac_sw,hvac_model,LV_ALIGN_OUT_RIGHT_MID,40,0);
    lv_obj_add_event_cb(hvac_sw,event_hvac,LV_EVENT_CLICKED,NULL);
    
    lv_obj_t* label_hvac = lv_label_create(obj5);
    lv_obj_set_style_text_font(label_hvac,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_hvac,"空调");
    lv_obj_set_size(label_hvac,60,50);
    lv_obj_align_to(label_hvac,hvac_sw,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    
    //温度
    label_temp = lv_label_create(obj5);
    lv_obj_set_style_text_font(label_temp,&kai30,LV_STATE_DEFAULT);
    hvac_temp = 20;
    lv_label_set_text(label_temp,"20");
    lv_obj_set_size(label_temp,30,30);
    lv_obj_align_to(label_temp,hvac_model,LV_ALIGN_OUT_BOTTOM_MID,0,10);

    //加减部件
    lv_obj_t* temp_up = lv_btn_create(obj5);
    lv_obj_set_size(temp_up,30,30);
    lv_obj_align_to(temp_up,label_temp,LV_ALIGN_OUT_RIGHT_MID,0,0);
    lv_obj_set_style_bg_img_src(temp_up,LV_SYMBOL_PLUS,LV_PART_MAIN);
    lv_obj_add_event_cb(temp_up,event_hvac_up,LV_EVENT_CLICKED,NULL);
    
    lv_obj_t* temp_down = lv_btn_create(obj5);
    lv_obj_set_size(temp_down,30,30);
    lv_obj_align_to(temp_down,label_temp,LV_ALIGN_OUT_LEFT_MID,0,0);
    lv_obj_set_style_bg_img_src(temp_down,LV_SYMBOL_MINUS,LV_PART_MAIN);
    lv_obj_add_event_cb(temp_down,event_hvac_down,LV_EVENT_CLICKED,NULL);


    //空调模式二级界面设置
    hvac_bg = lv_img_create(bg);
    lv_img_set_src(hvac_bg,&hv_bg);
    lv_obj_set_size(hvac_bg,800,480);
    lv_obj_set_pos(hvac_bg,0,0);
    lv_obj_add_flag(hvac_bg, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* cool_down = lv_img_create(hvac_bg);
    lv_img_set_src(cool_down,&cooler);
    lv_obj_set_style_img_recolor(cool_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(cool_down,200,LV_PART_MAIN);
    lv_obj_set_pos(cool_down,40,100);
    lv_obj_set_size(cool_down,150,150);
    
    cool_up = lv_img_create(hvac_bg);
    lv_img_set_src(cool_up,&cooler);
    lv_obj_set_pos(cool_up,40,100);
    lv_obj_set_size(cool_up,150,150);
    lv_obj_add_flag(cool_up, LV_OBJ_FLAG_HIDDEN);
   
    lv_obj_t* label_cool = lv_label_create(hvac_bg);
    lv_obj_set_style_text_font(label_cool,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_cool,"制冷模式");
    lv_obj_set_size(label_cool,150,50);
    lv_obj_align_to(label_cool,cool_down,LV_ALIGN_OUT_BOTTOM_MID,0,10);
 
    cool_sw = lv_switch_create(hvac_bg);
    lv_obj_set_size(cool_sw,100,50);
    lv_obj_align_to(cool_sw,label_cool,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    lv_obj_add_event_cb(cool_sw,event_cool,LV_EVENT_CLICKED,NULL);
    //设置为不可开状态，打开空调时解开
    lv_obj_add_state(cool_sw, LV_STATE_DISABLED); 

    lv_obj_t* hot_down = lv_img_create(hvac_bg);
    lv_img_set_src(hot_down,&hotter);
    lv_obj_set_style_img_recolor(hot_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(hot_down,200,LV_PART_MAIN);
    lv_obj_set_pos(hot_down,230,100);
    lv_obj_set_size(hot_down,150,150);
    
    hot_up = lv_img_create(hvac_bg);
    lv_img_set_src(hot_up,&hotter);
    lv_obj_set_pos(hot_up,230,100);
    lv_obj_set_size(hot_up,150,150);
    lv_obj_add_flag(hot_up, LV_OBJ_FLAG_HIDDEN);
   
    lv_obj_t* label_hot = lv_label_create(hvac_bg);
    lv_obj_set_style_text_font(label_hot,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_hot,"制热模式");
    lv_obj_set_size(label_hot,150,50);
    lv_obj_align_to(label_hot,hot_down,LV_ALIGN_OUT_BOTTOM_MID,0,10);
 
    hot_sw = lv_switch_create(hvac_bg);
    lv_obj_set_size(hot_sw,100,50);
    lv_obj_align_to(hot_sw,label_hot,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    lv_obj_add_event_cb(hot_sw,event_hot,LV_EVENT_CLICKED,NULL);

    lv_obj_add_state(hot_sw, LV_STATE_DISABLED);

    lv_obj_t* airy_down = lv_img_create(hvac_bg);
    lv_img_set_src(airy_down,&airy);
    lv_obj_set_style_img_recolor(airy_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(airy_down,200,LV_PART_MAIN);
    lv_obj_set_pos(airy_down,420,100);
    lv_obj_set_size(airy_down,150,150);
    
    airy_up = lv_img_create(hvac_bg);
    lv_img_set_src(airy_up,&airy);
    lv_obj_set_pos(airy_up,420,100);
    lv_obj_set_size(airy_up,150,150);
    lv_obj_add_flag(airy_up, LV_OBJ_FLAG_HIDDEN);
   
    lv_obj_t* label_airy = lv_label_create(hvac_bg);
    lv_obj_set_style_text_font(label_airy,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_airy,"通风模式");
    lv_obj_set_size(label_airy,150,50);
    lv_obj_align_to(label_airy,airy_down,LV_ALIGN_OUT_BOTTOM_MID,0,10);
 
    airy_sw = lv_switch_create(hvac_bg);
    lv_obj_set_size(airy_sw,100,50);
    lv_obj_align_to(airy_sw,label_airy,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    lv_obj_add_event_cb(airy_sw,event_airy,LV_EVENT_CLICKED,NULL);

    lv_obj_add_state(airy_sw, LV_STATE_DISABLED);

    lv_obj_t* desi_down = lv_img_create(hvac_bg);
    lv_img_set_src(desi_down,&desiccant);
    lv_obj_set_style_img_recolor(desi_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(desi_down,200,LV_PART_MAIN);
    lv_obj_set_pos(desi_down,610,100);
    lv_obj_set_size(desi_down,150,150);
    
    desi_up = lv_img_create(hvac_bg);
    lv_img_set_src(desi_up,&desiccant);
    lv_obj_set_pos(desi_up,610,100);
    lv_obj_set_size(desi_up,150,150);
    lv_obj_add_flag(desi_up, LV_OBJ_FLAG_HIDDEN);
   
    lv_obj_t* label_desi = lv_label_create(hvac_bg);
    lv_obj_set_style_text_font(label_desi,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_desi,"除湿模式");
    lv_obj_set_size(label_desi,150,50);
    lv_obj_align_to(label_desi,desi_down,LV_ALIGN_OUT_BOTTOM_MID,0,10);
 
    desi_sw = lv_switch_create(hvac_bg);
    lv_obj_set_size(desi_sw,100,50);
    lv_obj_align_to(desi_sw,label_desi,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    lv_obj_add_event_cb(desi_sw,event_desi,LV_EVENT_CLICKED,NULL);
    
    lv_obj_add_state(desi_sw, LV_STATE_DISABLED);

    //退出图标事件
    lv_obj_t* back = lv_btn_create(hvac_bg);
    lv_obj_set_size(back,100,50);
    lv_obj_t* label_back = lv_label_create(back);
    lv_obj_set_style_text_font(back,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_back,"返回");
    lv_obj_align_to(back,cool_sw,LV_ALIGN_OUT_BOTTOM_MID,200,10);
    lv_obj_add_event_cb(back,event_hvac_back,LV_EVENT_CLICKED,NULL);


    //总灯控
    lv_obj_t* lamp_down = lv_img_create(obj3);
    lv_img_set_src(lamp_down,&lamp_all);
    lv_obj_set_style_img_recolor(lamp_down,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(lamp_down,200,LV_PART_MAIN);
    lv_obj_set_pos(lamp_down,-20,-30);
    lv_obj_set_size(lamp_down,100,150);

    lamp_up = lv_img_create(obj3);
    lv_img_set_src(lamp_up,&lamp_all);
    lv_obj_set_pos(lamp_up, -20,-30);
    lv_obj_set_size(lamp_up,100,150);
    lv_obj_add_flag(lamp_up, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* label_lp = lv_label_create(obj3);
    lv_obj_set_style_text_font(label_lp,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lp,"灯光");
    lv_obj_set_size(label_lp,60,50);
    lv_obj_align_to(label_lp,lamp_down,LV_ALIGN_OUT_BOTTOM_MID,0,0);

    //模式
    lv_obj_t* lamp_model = lv_btn_create(obj3);
    lv_obj_set_size(lamp_model,100,50);
    lv_obj_align_to(lamp_model,lamp_down,LV_ALIGN_OUT_RIGHT_MID,10,-40);
    lv_obj_t* label_model = lv_label_create(lamp_model);
    lv_obj_set_style_text_font(label_model,&kai30,LV_STATE_DEFAULT);

    lv_label_set_text(label_model,"设置");
    lv_obj_add_event_cb(lamp_model,event_lamp_model,LV_EVENT_CLICKED,NULL); //设置按钮事件

    lv_obj_t* all_up = lv_btn_create(obj3);
    lv_obj_set_size(all_up,100,50);
    lv_obj_t* label_up = lv_label_create(all_up);
    lv_obj_set_style_text_font(all_up,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_up,"全开");
    lv_obj_align_to(all_up,lamp_down,LV_ALIGN_OUT_RIGHT_MID,10,20);
    lv_obj_add_event_cb(all_up,event_all_up,LV_EVENT_CLICKED,NULL);

    lv_obj_t* all_down = lv_btn_create(obj3);
    lv_obj_set_size(all_down,100,50);
    lv_obj_t* label_down = lv_label_create(all_down);
    lv_obj_set_style_text_font(all_down,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_down,"全关");
    lv_obj_align_to(all_down,all_up,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    lv_obj_add_event_cb(all_down,event_all_down,LV_EVENT_CLICKED,NULL);

    //灯控二级界面
    lp_bg = lv_img_create(bg);
    lv_img_set_src(lp_bg,&lamp_bg);
    lv_obj_set_size(lp_bg,800,480);
    lv_obj_set_pos(lp_bg,0,0);
    lv_obj_add_flag(lp_bg, LV_OBJ_FLAG_HIDDEN);

    //退出图标事件
    lv_obj_t* lp_back = lv_btn_create(lp_bg);
    lv_obj_set_size(lp_back,100,50);
    lv_obj_t* label_lamp_back = lv_label_create(lp_back);
    lv_obj_set_style_text_font(lp_back,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lamp_back,"返回");
    lv_obj_set_pos(lp_back,350,400);
    lv_obj_add_event_cb(lp_back,event_lamp_back,LV_EVENT_CLICKED,NULL);

    //lamp二级界面
    lv_obj_t* lamp_down1 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_down1,&lamp);
    lv_obj_set_style_img_recolor(lamp_down1,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(lamp_down1,200,LV_PART_MAIN);
    lv_obj_set_pos(lamp_down1,25,60);
    lv_obj_set_size(lamp_down1,150,200);
    
    lamp_up1 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_up1,&lamp);
    lv_obj_set_pos(lamp_up1, 25,60);
    lv_obj_set_size(lamp_up1,150,200);
    lv_obj_add_flag(lamp_up1, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_lp1 = lv_label_create(lp_bg);
    lv_obj_set_style_text_font(label_lp1,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lp1,"卧室");
    lv_obj_set_size(label_lp1,60,50);
    lv_obj_align_to(label_lp1,lamp_down1,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    
    lamp_sw1 = lv_switch_create(lp_bg);
    lv_obj_set_size(lamp_sw1,100,50);
    lv_obj_align_to(lamp_sw1,label_lp1,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(lamp_sw1,event_lamp1,LV_EVENT_CLICKED,NULL);


    lv_obj_t* lamp_down2 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_down2,&lamp);
    lv_obj_set_style_img_recolor(lamp_down2,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(lamp_down2,200,LV_PART_MAIN);
    lv_obj_set_pos(lamp_down2,225,60);
    lv_obj_set_size(lamp_down2,150,200);
    
    lamp_up2 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_up2,&lamp);
    lv_obj_set_pos(lamp_up2, 225,60);
    lv_obj_set_size(lamp_up2,150,200);
    lv_obj_add_flag(lamp_up2, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* label_lp2 = lv_label_create(lp_bg);
    lv_obj_set_style_text_font(label_lp2,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lp2,"客厅");
    lv_obj_set_size(label_lp2,60,50);
    lv_obj_align_to(label_lp2,lamp_down2,LV_ALIGN_OUT_BOTTOM_MID,0,10);
    
    lamp_sw2 = lv_switch_create(lp_bg);
    lv_obj_set_size(lamp_sw2,100,50);
    lv_obj_align_to(lamp_sw2,label_lp2,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(lamp_sw2,event_lamp2,LV_EVENT_CLICKED,NULL);


    lv_obj_t* lamp_down3 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_down3,&lamp);
    lv_obj_set_style_img_recolor(lamp_down3,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(lamp_down3,200,LV_PART_MAIN);
    lv_obj_set_pos(lamp_down3,425,60);
    lv_obj_set_size(lamp_down3,150,200);
    
    lamp_up3 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_up3,&lamp);
    lv_obj_set_pos(lamp_up3, 425,60);
    lv_obj_set_size(lamp_up3,150,200);
    lv_obj_add_flag(lamp_up3, LV_OBJ_FLAG_HIDDEN);
     
    lv_obj_t* label_lp3 = lv_label_create(lp_bg);
    lv_obj_set_style_text_font(label_lp3,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lp3,"厨房");
    lv_obj_set_size(label_lp3,60,50);
    lv_obj_align_to(label_lp3,lamp_down3,LV_ALIGN_OUT_BOTTOM_MID,0,10);
  
    lamp_sw3 = lv_switch_create(lp_bg);
    lv_obj_set_size(lamp_sw3,100,50);
    lv_obj_align_to(lamp_sw3,label_lp3,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(lamp_sw3,event_lamp3,LV_EVENT_CLICKED,NULL);

    lv_obj_t* lamp_down4 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_down4,&lamp);
    lv_obj_set_style_img_recolor(lamp_down4,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(lamp_down4,200,LV_PART_MAIN);
    lv_obj_set_pos(lamp_down4,625,60);
    lv_obj_set_size(lamp_down4,150,200);

    lamp_up4 = lv_img_create(lp_bg);
    lv_img_set_src(lamp_up4,&lamp);
    lv_obj_set_pos(lamp_up4, 625,60);
    lv_obj_set_size(lamp_up4,150,200);
    lv_obj_add_flag(lamp_up4, LV_OBJ_FLAG_HIDDEN);
     
    lv_obj_t* label_lp4 = lv_label_create(lp_bg);
    lv_obj_set_style_text_font(label_lp4,&kai30,LV_STATE_DEFAULT);
    lv_label_set_text(label_lp4,"书房");
    lv_obj_set_size(label_lp4,60,50);
    lv_obj_align_to(label_lp4,lamp_down4,LV_ALIGN_OUT_BOTTOM_MID,0,0);
  
    lamp_sw4 = lv_switch_create(lp_bg);
    lv_obj_set_size(lamp_sw4,100,50);
    lv_obj_align_to(lamp_sw4,label_lp4,LV_ALIGN_OUT_BOTTOM_MID,0,0);
    lv_obj_add_event_cb(lamp_sw4,event_lamp4,LV_EVENT_CLICKED,NULL);



}



int main(void)
{
    /*1、lvgl初始化---所有lvgl工程第一步做*/
    lv_init();

    /*2、输出设备LCD初始化及注册*/
    fbdev_init();//打开了/dev/fb0设备文件及其他操作
    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;//我们自己提供的当前LCD屏的画点函数
    disp_drv.hor_res    = 800;
    disp_drv.ver_res    = 480;
    lv_disp_drv_register(&disp_drv);//将LCD屏设备信息注册到lvgl内。让lvgl能够使用我们的屏幕

    //3、输入设备初始化及注册
    evdev_init();//打开/dev/input/event0设备文件及其他操作
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); 
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;
    indev_drv_1.read_cb = evdev_read;//读取触摸屏的x y坐标函数
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);//将以上准备好的触摸屏设备信息注册到lvgl,让lvgl能使用触摸屏

    led_fd = open(LED_DEV, O_RDWR);
	if(led_fd == -1)
	{
		printf("open failure\n");
		return -1;
	}

    beep_fd=-1;
    duty = 40;
    beep_fd = open("/dev/gec6818_beep",O_RDWR);
	
	if(beep_fd < 0)
	{
		perror("open /dev/beep:");
		
		return beep_fd;	
	}

    //4、执行代码
    UI_Start();
    
    
    //实时时间显示线程
    pthread_t lvgl_tid1;
    pthread_t lvgl_tid2;
    pthread_t lvgl_tid3;
    pthread_create(&lvgl_tid1, NULL, time_thread, NULL);
    pthread_create(&lvgl_tid2, NULL, remote_thread, NULL);
    pthread_create(&lvgl_tid3, NULL, get_weather, NULL);
    // 设置线程为分离状态
    pthread_detach(lvgl_tid1);
    pthread_detach(lvgl_tid3);
    
    
    /*5、事物处理及告知lvgl节拍数*/
    while(1) {
        lv_timer_handler();//事务处理
        lv_tick_inc(5);//节拍累计  5ms
        usleep(5000);
    }

    return 0;
}


/*用户节拍获取*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) 
    {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
