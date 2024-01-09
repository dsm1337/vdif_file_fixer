#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <cstring>
#define chosen_path 0

union word
{
    char bytes[4];
    unsigned int to_int;
};
struct loss_position
{
    unsigned int begin; // position of package before gap
    unsigned int end;   // position of package after gap 
};
using namespace std;
bool get_header(istream *input, word vdif_header[8])
{
    bool eof = false;
    for (int i = 0; i < 8; i++)
    {
        if (!(*input).read(vdif_header[i].bytes,4))
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
    if (input.fail())
    {
        cerr << "Could not open file: " << path_in << endl;
    }
    int sec = 0;
    int prev_number_df = 0;
    bool eof = false;
    bool long_time_no_data = false;
    vector<int> pos;
    float FPS = 0;
    int number_of_lost_packages = 0;
    while(!eof)
    {
        word vdif_header[8];
        eof = get_header(&input, vdif_header);
        if(!eof)
        {
            int number_df = vdif_header[1].to_int & 0x7FFFFF;
            cout << number_df << ":\n";
            int cur_sec = vdif_header[0].to_int & 0x3FFFFFFF;
            cout << "Seconds:" << cur_sec << "\t";
            int frame_len = (vdif_header[2].to_int & 0xffffff) * 8;
            cout << "Frame length:" << frame_len << "\t";
            if(cur_sec != sec)
            {
                FPS = get_FPS(vdif_header);
            }
            cout << "FPS:" << FPS << endl << flush;
            if(vdif_header[0].to_int >> 31 == 1)
            {
                cout << "invalid" << endl;
            }
            if(number_df - prev_number_df > 1)
            {
                number_of_lost_packages += number_df - prev_number_df;
            }
            else if ((prev_number_df > number_df) && (prev_number_df < FPS - 1))
            {
                number_of_lost_packages += FPS + number_df - prev_number_df - 1;
            }
            if((cur_sec - sec > 20))
            {
                if (sec == 0)
                {
                    sec = cur_sec;
                }
                else
                {
                    long_time_no_data = true;
                    pos.push_back(input.tellg() - 32);
                }
            }
            sec = cur_sec;
            input.seekg(frame_len - 32,ios::cur);
            prev_number_df = number_df;
        }
    }
    input.close();
    if(long_time_no_data)
    {
        cout << "data bad" << endl;
        for (int i=0; i<pos.size(); i++) cout<< pos[i] << endl;
        fstream vdif_file;
        ofstream output, output2;
        word data;
        vdif_file.open(path_in, ios::binary | ios::in);
        if (vdif_file.fail())
        {
            cerr << "Could not open file: " << path_in << endl;
        }
        for (int i = 0; i < pos.size();i++) 
        {
            word header[8];
            output.open(path_out + to_string(i) + ".vdif", ios::binary | ios::out);
            if (output.fail())
            {
                cerr << "Could not open file: " << path_out << i << endl;
            }
            cout <<  i  <<": ";
            while (vdif_file.tellg() < pos[i])
            {
                vdif_file.read(data.bytes,4);
                output.write(data.bytes,4);
            }
            output.close();
            cout << " completed" << endl;
        }
        output.open(path_out + to_string(pos.size()) +".vdif", ios::binary | ios::out);
        if (output.fail())
        {
            cerr << "Could not open file: " << path_out << pos.size() << endl;
        }
        cout <<  pos.size()<< ":" << endl;
        while (vdif_file.read(data.bytes,4))
        {
            output.write(data.bytes,4);
        }
        output.close();
        word header[8];
        vdif_file.close(); 
    }
    else
    {
        cout << "data good" << endl;
    }
    cout << "Number of lost packages:" << number_of_lost_packages << endl;
    cout << "\nEnd of Program" << endl;
    return 0;
}