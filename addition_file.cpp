#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <iterator>
#define chosen_path 1

union word
{
    char bytes[4];
    unsigned int to_int;
};
struct loss_position
{
    unsigned int begin;
    unsigned int end;
};
using namespace std;
void append_empty(ofstream &out, word header[8], int end_number_df)
{
    int frame_len = (header[2].to_int & 0xffffff) * 8;
    int number_of_empty_fields = (int)end_number_df - header[1].to_int & 0x7FFFFF - 1;
    header[1].to_int +=1;
    for (; (header[1].to_int& 0x7FFFFF) < end_number_df; header[1].to_int++)
    {
        cout << "append\t" << (header[1].to_int & 0x7FFFFF ) <<endl;
        if ((header[1].to_int & 0x7FFFFF )> 49999) 
        {
            cout << end_number_df << " " << number_of_empty_fields<< " i: "<< endl;
        }

        for(int j = 0; j < 8; j++) 
        {
            out.write(header[j].bytes, 4);
        }
        char null_byte = 0;
        for (int j = 0; j < (frame_len - 32); j++)
        {
            out.write(&null_byte,1);
        }
    }
    // getchar();
}
void prepend_empty(ofstream &out, word header[8])
{
    int frame_len = (header[2].to_int & 0xffffff) * 8;
    word number_df;
    for (number_df.to_int = header[1].to_int & 0xFF800000; number_df.to_int < header[1].to_int ; number_df.to_int++)
    {
        
        char null_byte = 0;
        cout << (number_df.to_int & 0x7FFFFF) << " prepend\n";
        for(int j = 0; j < 8; j++) 
        {
            if(j != 1)out.write(header[j].bytes, 4);
            else out.write( number_df.bytes, 4);
        }
        for (int j = 0; j < frame_len - 32; j++)
        {
            out.write(&null_byte,1);
        } 
    }
}
bool get_header(istream *input, word header[8])
{
    bool eof = false;
    for (int i = 0; i < 8; i++)
    {
        if (!(*input).read(header[i].bytes,4))
        {
            eof = true;
            break;
        }
    }
    return eof;
}
float get_FPS(word header[8])
{
    int bits = ((header[3].to_int >> 26) & 0x1f) + 1;
    int srate = header[4].to_int & 0x7fffff;
    int channels_num = (0x1 << ((header[2].to_int  >> 24) & 0x1f));
    int data_rate = srate * bits * channels_num;
    int frame_len = (header[2].to_int & 0xffffff) * 8;
    float FPS = (data_rate * 125000.) / (frame_len - 32);
    return FPS;
}
int sec = 0;
int main (int argc, char* argv[])
{
    cout << "Start" << endl;
    ifstream input;
    string path_in, path_out;
    if(argc > 1)
    {
        path_in = argv[1];
        if(argc > 2) 
        {
            path_out = argv[2];
        }
        else
        {
            path_out = "appended_file.vdif";
        }
        cout<< path_in << "\t" << path_out<<endl; 
    }
    else
    {
        string paths[6] = {"/home/yakubov/coding/r7296b_zv_296-0439_ch01_bad.vdif",
                       "/home/yakubov/coding/r7296b_zv_296-0439_ch01_bad_new_0.vdif",
                       "/home/yakubov/coding/r7296b_zv_296-0439_ch01_bad_new_1.vdif",
                       "/home/yakubov/coding/r7296b_zv_296-0439_ch02_bad.vdif",
                       "/home/yakubov/coding/r7296b_zv_296-0436a_ch01.vdif",
                       "/home/yakubov/coding/good_file/r7296b_zv_296-0439_ch01_bad.vdif"};
        path_in = paths[chosen_path];
        path_out = "/home/yakubov/coding/";
    }
    input.open(path_in, ios::binary | ios::in);
    int prev_number_df = 0;
    bool eof = false;
    bool long_time_no_data = false;
    unsigned int pos = 0;
    ofstream output;
    int number_of_lost_packages = 0;
    int sec = 0;
    word previous_header[8];
    previous_header[1].to_int = 0;
    output.open(path_out, ios::binary);
    while(!eof)
    {
        loss_position buf;
        word header[8];
        eof = get_header(&input, header);
        if(!eof)
        {
            int number_df = header[1].to_int & 0x7FFFFF;
            cout << number_df << ":\t";
            int frame_len = (header[2].to_int & 0xffffff) * 8;
            int cur_sec = header[0].to_int & 0x3FFFFFFF;
            if (number_df > prev_number_df + 1)
            {
                if(sec != cur_sec)
                {
                    float FPS = get_FPS(header);
                    append_empty(output,previous_header,(int)FPS);
                    prepend_empty(output,header);
                }
                else append_empty(output,previous_header, number_df);
            }
            else if(number_df < prev_number_df)
            {
                float FPS = get_FPS(header);
                append_empty(output,previous_header,(int)FPS);
                if(number_df != 0)prepend_empty(output, header);
            }
            cout << header[1].to_int <<endl;
            
            for (int i = 0; i < 8; i++)
            {
                output.write(header[i].bytes,4);
            }
            for (int i = 0; i < frame_len - 32; i+=8)
            {
                char buf[8];
                input.read(buf, 8);
                output.write(buf, 8);
            }
            sec = cur_sec;
            prev_number_df = number_df;
            for (int i = 0; i < 8; i++)
            {
                previous_header[i] = header[i];
            }
            cout <<( header[1].to_int & 0x7FFFFF )<< endl;
        }
    }
    input.close();
    float FPS = get_FPS(previous_header);
    if((previous_header[1].to_int & 0x7FFFFF )< (int)FPS - 1)
    {
        append_empty(output,previous_header,(int)FPS);
    }
    output.close();
    cout << "\nEnd of Program" << endl;
    return 0;
}