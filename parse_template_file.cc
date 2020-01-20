#include "template_feature.pb.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include "st_imagehelper.hpp"
#include "codec/jpge.h"
#include "codec/stb_image.h"

// #define TO_STRING(x) #x

const char* template_file_path = "./template_file.data";
const char* parsed_template_dir = "./parsed_templates/template_";

const char* template_version_1_0_0 = "1.0.0";
const char* template_version_1_0_1 = "1.0.1";

typedef struct st_tee_input_t {
    unsigned char *image_data;
    int width;
    int height;
    int channel;
    int format;
    //st_face_orientation orientation;
} st_tee_input;


template < typename T > 
std::string to_string(const T& n)
{
	std::ostringstream stm ;
	stm << n ;
	return stm.str() ;
}


bool IsFileExist(const char* path)
{
	if (nullptr == path) return false;
	if (access(path, F_OK) == -1) {
		return false;
	}

	return true;
}


int ReadFile(const char *filePath, char **content, int &nFileLen)
{
	FILE *pF = NULL;
    pF = fopen(filePath, "r");
    if (pF == NULL) {
        return -1;
    }
    fseek(pF, 0, SEEK_END);
    nFileLen = ftell(pF);
    rewind(pF);
    char *szBuffer = new (std::nothrow) char[nFileLen + 1];
    if (!szBuffer) {
        fclose(pF);
        return -1;
    }
    nFileLen = fread(szBuffer, sizeof(char), nFileLen, pF);
    szBuffer[nFileLen] = '\0';
    fclose(pF);
    *content = szBuffer;
    return 0;
}


int WriteFile(const char *path, const char *file, int length)
{
	if (nullptr == path || nullptr == file || length <= 0) 
	{
		printf("Warning ! %s invalid args\n", __FUNCTION__);
		return -1;
	}

	std::ofstream out(path, std::fstream::out | std::fstream::trunc);
	if (!out.is_open()) 
	{
		printf("Error ! template file [%s] open failed\n", path);
		return -1;
	}

	out.write(file, length);
	out.close();

	return 0;
}

int LoadJpegMemoryToBGR(const unsigned char *srcBuffer, 
					int srcBufferLen, 
					int &width, int &height, int &channels, 
					std::unique_ptr< unsigned char[] > &outBufferGurad) 
{
	int ret(0);
    if (!srcBuffer) {
        return -1;
    }
    unsigned char *readBuffer = stbi_load_from_memory(( unsigned char * )srcBuffer, srcBufferLen, &width, &height, &channels, STBI_rgb);
    if (readBuffer == nullptr) {
        printf("image load_from_memory failed\n");
        return -1;
    }
    printf("load from memory width:%d height:%d channels:%d\n", width, height, channels);
    auto stbiImageFree = [](unsigned char *pInv) { stbi_image_free(pInv); };
    std::unique_ptr< unsigned char, decltype(stbiImageFree) > detectionResultManager(readBuffer, stbiImageFree);

    auto mallocLen = width * height * channels;
    unsigned char *dstRGBBuffer = new (std::nothrow) unsigned char[mallocLen];
    if (dstRGBBuffer == nullptr) {
        return -1;
    }
    outBufferGurad.reset(dstRGBBuffer);
    memcpy(dstRGBBuffer, readBuffer, mallocLen);
    return ret;
}

int main()
{	
	std::shared_ptr< pb::TemplateFile > parse_template = std::make_shared<pb::TemplateFile>();

	if (!IsFileExist(template_file_path)){
		printf("Error ! template file not exist!\n");
		return 0;
	}

	char *template_file_content(nullptr);
	int file_length(0);
	std::unique_ptr< char[] > templateFileContentManager;

	if (0 != ReadFile(template_file_path, &template_file_content, file_length)) {
		printf("Error ! read template file failed!\n");
		return 0;
	}
	
	templateFileContentManager.reset(template_file_content);

	if (nullptr == template_file_content) {
		printf("Error ! template file is NULL\n");
		return 0;
	} else {
		bool ret(false);
		ret = parse_template->ParseFromArray(template_file_content, file_length);
		if (!ret) {
			printf("Error ! parse template from array failed!\n");
			return 0;
		} else {
			printf("parse template from array SUCCESS\n");
		}
	}


	printf("single person template size = %d\n", parse_template->singlepersontemplate_size());
	printf("modelversion : %d\n", parse_template->modelversion());
	printf("versionstring : %s\n", parse_template->versionstring().c_str());
	printf("identifier : %s\n", parse_template->identifier().c_str());
	printf("singlePersonTemplateIndex : %d\n", parse_template->singlepersontemplateindex());

	std::vector<st_tee_input> parsed_templates;

	auto tempalte_version = parse_template->versionstring();
	int person_count = parse_template->singlepersontemplate_size();
	for (auto i = 0; i < person_count; i++)
	{
		pb::SinglePersonTemplate &singlePersonTemplate = const_cast<pb::SinglePersonTemplate&>(parse_template->singlepersontemplate(i));
		auto single_template_count = singlePersonTemplate.singletemlate_size();
		for (auto j = 0; j < single_template_count; j++)
		{
			pb::SingleTemlate &single_template = const_cast<pb::SingleTemlate&>(singlePersonTemplate.singletemlate(j));
			auto image = single_template.imageinfo();
			st_tee_input parsed_image {0};
			std::unique_ptr<unsigned char[]> outBufferGuard;
			if (tempalte_version == template_version_1_0_0)
			{
				parsed_image.image_data = (unsigned char*)(image.data().c_str());
				parsed_image.width = image.width();
				parsed_image.height = image.height();
				parsed_image.format = image.format();
				parsed_image.channel = 0;
			}
			else if (tempalte_version == template_version_1_0_1)
			{
				int width = 0;
				int height = 0;
				int channels = 0;
				
				printf("image.data().length() = %d\n", image.data().length());

				if (0 != LoadJpegMemoryToBGR(reinterpret_cast<const unsigned char*>(image.data().c_str()), 
											image.data().length(), width, height, channels,
											outBufferGuard))
				{
					printf("Error ! LoadJpegMemoryToBGR failed!\ns");
					return 0;
				}
				parsed_image = { outBufferGuard.get(), image.width(), image.height(), channels, 0 };
				parsed_image.format = image.format();
				// parsed_templates.push_back(parsed_image);

				std::string parsed_template_path(parsed_template_dir);
				parsed_template_path = parsed_template_path + ::to_string(i);
				printf("%s\n", parsed_template_path.c_str());
				const char* str = reinterpret_cast<const char*>(parsed_image.image_data);
				int length = width * height * channels;
				printf("length = %d\n", length);
				WriteFile(parsed_template_path.c_str(), str, length);

			}
			else
			{
				printf("Error ! invalid template version\n");
				return 0;
			}


		}
		
	}
	


	return 0;
}
