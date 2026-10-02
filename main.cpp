#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

#include <hpdf.h>

// 将任意 Poppler 图像转换为紧凑的 RGB24 字节数组（img 非 const，因为 data() 非 const）
static std::vector<unsigned char> image_to_rgb_data(poppler::image &img) {
    int width = img.width();
    int height = img.height();
    int bytes_per_row = img.bytes_per_row();
    const unsigned char *src = reinterpret_cast<const unsigned char*>(img.data());
    std::vector<unsigned char> rgb(width * height * 3);

    if (img.format() == poppler::image::format_rgb24) {
        // 直接拷贝，去掉可能的行填充
        for (int y = 0; y < height; ++y) {
            std::memcpy(rgb.data() + y * width * 3, src + y * bytes_per_row, width * 3);
        }
    } else if (img.format() == poppler::image::format_argb32) {
        // ARGB32 布局通常为 BGRA（小端）
        for (int y = 0; y < height; ++y) {
            const unsigned char *row = src + y * bytes_per_row;
            unsigned char *dst = rgb.data() + y * width * 3;
            for (int x = 0; x < width; ++x) {
                unsigned char b = row[x * 4 + 0];
                unsigned char g = row[x * 4 + 1];
                unsigned char r = row[x * 4 + 2];
                // 忽略 row[x*4+3] (alpha)
                dst[x * 3 + 0] = r;
                dst[x * 3 + 1] = g;
                dst[x * 3 + 2] = b;
            }
        }
    } else if (img.format() == poppler::image::format_bgr24) {
        // BGR24，交换 R 和 B
        for (int y = 0; y < height; ++y) {
            const unsigned char *row = src + y * bytes_per_row;
            unsigned char *dst = rgb.data() + y * width * 3;
            for (int x = 0; x < width; ++x) {
                dst[x * 3 + 0] = row[x * 3 + 2]; // R
                dst[x * 3 + 1] = row[x * 3 + 1]; // G
                dst[x * 3 + 2] = row[x * 3 + 0]; // B
            }
        }
    } else if (img.format() == poppler::image::format_mono) {
        // 灰度图，扩展为 RGB
        for (int y = 0; y < height; ++y) {
            const unsigned char *row = src + y * bytes_per_row;
            unsigned char *dst = rgb.data() + y * width * 3;
            for (int x = 0; x < width; ++x) {
                unsigned char gray = row[x]; // 假设单字节
                dst[x * 3 + 0] = gray;
                dst[x * 3 + 1] = gray;
                dst[x * 3 + 2] = gray;
            }
        }
    } else {
        throw std::runtime_error("Unsupported Poppler image format.");
    }
    return rgb;
}

static void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
    std::cerr << "libHaru error: " << error_no << ", detail: " << detail_no << std::endl;
    throw std::runtime_error("libHaru failed.");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input.pdf output.pdf" << std::endl;
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argv[2];

    try {
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(input_path));
        if (!doc || doc->is_locked()) {
            throw std::runtime_error("Failed to open or locked PDF: " + input_path);
        }

        const int page_count = doc->pages();
        std::cout << "PDF pages: " << page_count << std::endl;

        poppler::page_renderer renderer;
        renderer.set_render_hint(poppler::page_renderer::antialiasing, true);
        renderer.set_render_hint(poppler::page_renderer::text_antialiasing, true);
        const double dpi = 200.0;

        HPDF_Doc pdf = HPDF_New(error_handler, nullptr);
        if (!pdf) {
            throw std::runtime_error("Cannot create PDF document.");
        }
        HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);

        for (int i = 0; i < page_count; ++i) {
            std::unique_ptr<poppler::page> p(doc->create_page(i));
            if (!p) {
                std::cerr << "Warning: page " << i << " is empty, skipping." << std::endl;
                continue;
            }

            poppler::image img = renderer.render_page(p.get(), dpi, dpi);
            int width = img.width();
            int height = img.height();

            // 获取紧凑的 RGB24 数据（内部根据格式自动转换）
            std::vector<unsigned char> rgb = image_to_rgb_data(img);

            HPDF_Image hpdf_img = HPDF_LoadRawImageFromMem(
                pdf, rgb.data(), width, height,
                HPDF_CS_DEVICE_RGB, 8
            );
            if (!hpdf_img) {
                throw std::runtime_error("Failed to create Haru image from data.");
            }

            double page_width = width * 72.0 / dpi;
            double page_height = height * 72.0 / dpi;

            HPDF_Page page = HPDF_AddPage(pdf);
            HPDF_Page_SetWidth(page, page_width);
            HPDF_Page_SetHeight(page, page_height);
            HPDF_Page_DrawImage(page, hpdf_img, 0, 0, page_width, page_height);

            std::cout << "Processed page " << (i + 1) << "/" << page_count
                      << " (" << width << "x" << height << " px)" << std::endl;
        }

        HPDF_SaveToFile(pdf, output_path.c_str());
        HPDF_Free(pdf);
        std::cout << "Successfully created: " << output_path << std::endl;
    }
    catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
