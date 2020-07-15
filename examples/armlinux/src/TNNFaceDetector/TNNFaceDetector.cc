// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the 
// specific language governing permissions and limitations under the License.

#include <fstream>
#include <string>
#include <vector>

#include "UltraFaceDetector.h"
#include "TNNSDKSample.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../third_party/stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../../../third_party/stb/stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../../../third_party/stb/stb_image_write.h"

void *MallocWrap(int byte_num){
    void *MallocBuf = (void *)malloc(byte_num);
    if(MallocBuf == NULL)
        printf("\n Malloc Failed.\n");
    return MallocBuf;	
}

// Helper functions
std::string fdLoadFile(std::string path) {
    std::ifstream file(path, std::ios::in);
    if (file.is_open()) {
        file.seekg(0, file.end);
        int size      = file.tellg();
        char* content = new char[size];
        file.seekg(0, file.beg);
        file.read(content, size);
        std::string fileContent;
        fileContent.assign(content, size);
        delete[] content;
        file.close();
        return fileContent;
    } else {
        return "";
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("how to run:  %s proto model height width\n", argv[0]);
        return -1;
    }
    // 创建tnn实例
    int h = 240, w = 320;
    if(argc >= 5) {
        h = std::atoi(argv[3]);
        w = std::atoi(argv[4]);
    }
    //UltraFaceDetector detector(target_width, target_height, 1, 0.95, 0.15);
    std::shared_ptr<UltraFaceDetector>  detector = std::make_shared<UltraFaceDetector>(w, h, 1, 0.95, 0.15);
    std::vector<int> nchw = {1, 3, h, w};

    auto proto = fdLoadFile(argv[1]);
    auto model = fdLoadFile(argv[2]);

    char img_buff[256];
    char *input_imgfn = img_buff;
    if(argc < 6)
        strncpy(input_imgfn, "../../assets/test_face.jpg", 256);
    else
        strncpy(input_imgfn, argv[5], 256);
    printf("Face-detector is about to start, and the picrture is %s\n",input_imgfn);
    int image_width, image_height, image_channel;
    unsigned char *data = stbi_load(input_imgfn, &image_width, &image_height, &image_channel, 3);
    printf("The origin image_width:%d \n",int(image_width));
    printf("The origin image_height:%d \n",int(image_height));
    printf("The origin image_channel:%d \n",int(image_channel));

    CHECK_TNN_STATUS(detector->Init(proto, model, "", TNN_NS::TNNComputeUnitsCPU));
    auto input_mat = std::make_shared<TNN_NS::Mat>(TNN_NS::DEVICE_ARM, TNN_NS::N8UC3, nchw, data);

    std::vector<FaceInfo> face_info;
    CHECK_TNN_STATUS(detector->Detect(input_mat, h, w, face_info));
    printf("Detect done!\n");

    const int image_orig_height = int(image_height);
    const int image_orig_width  = int(image_width);
    float scale_x               = image_orig_width / (float)w;
    float scale_y               = image_orig_height / (float)h;

    //convert rgb to rgb-a
    uint8_t *ifm_buf = (uint8_t *)MallocWrap(320*240*4);
    for (int i = 0; i < 320*240; ++i) {
        ifm_buf[i*4] = data[i*3];
        ifm_buf[i*4+1] = data[i*3+1];
        ifm_buf[i*4+2] = data[i*3+2];
        ifm_buf[i*4+3] = 255;
    }
    for (int i = 0; i < face_info.size(); i++) {
        auto face = face_info[i];
        TNN_NS::Rectangle((void *)ifm_buf, image_orig_height, image_orig_width, face.x1, face.y1, face.x2,
                  face.y2, scale_x, scale_y);
    }

    char buff[256];
    sprintf(buff, "%s.png", "predictions");
    int success = stbi_write_bmp(buff, image_orig_width, image_orig_height, 4, ifm_buf);
    if(!success) 
        return -1;

    fprintf(stdout, "Face-detector done.\nNumber of faces: %d\n",int(face_info.size()));
    free(ifm_buf);
    free(data);
    return 0;
}
