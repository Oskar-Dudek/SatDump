#include "overlay_handler.h"
#include "core/config.h"

int OverlayHandler::enabled()
{
    return (int)draw_map_overlay +
           (int)draw_cities_overlay +
           (int)draw_qth_overlay +
           (int)draw_latlon_overlay +
           (int)draw_shores_overlay;
}

bool OverlayHandler::drawUI()
{
    bool update = false;
    ImGui::Checkbox("Lat/Lon Grid", &draw_latlon_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##latlongrid", (float *)&color_latlon, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("Map Overlay##Projs", &draw_map_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##borders", (float *)&color_borders, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("Shores Overlay##Projs", &draw_shores_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##shores", (float *)&color_shores, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("Cities Overlay##Projs", &draw_cities_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##cities##Projs", (float *)&color_cities, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("QTH Overlay##Projs", &draw_qth_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##qth##Projs", (float *)&color_qth, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::IsItemDeactivatedAfterEdit();

    widgets::SteppedSliderInt("Map Labels Font Size", &cities_size, 10, 500);
    static const char *city_categories[] = {"Capitals Only", "Capitals + Regional Capitals", "All (by Scale Rank)"};
    ImGui::Combo("Cities Type##Projs", &cities_type, city_categories, IM_ARRAYSIZE(city_categories));
    if (cities_type == 2)
        widgets::SteppedSliderInt("Cities Scale Rank", &cities_scale_rank, 0, 10);

    if (ImGui::Button("Set Defaults###oerlayhandlers"))
        set_defaults();
    ImGui::SameLine();
    if (ImGui::Button("Apply###overlayhandlerapply"))
        update = true;

    return update;
}

void OverlayHandler::apply(image::Image<uint16_t> &img, std::function<std::pair<int, int>(float, float, int, int)> &proj_func, float *step_cnt)
{
    const size_t width = img.width();
    const size_t height = img.height();
    // Draw map borders
    if (draw_map_overlay)
    {
        if (map_cache == nullptr || map_cache->width() != width || map_cache->height() != height ||
            last_color_borders.w != color_borders.w || last_color_borders.x != color_borders.x ||
            last_color_borders.y != color_borders.y || last_color_borders.z != color_borders.z)
        {
            logger->info("Drawing map overlay...");
            delete map_cache;
            map_cache = new image::Image<uint16_t>(width, height, 4);
            unsigned short color[4] = { (unsigned short)(color_borders.x * 65535.0f), (unsigned short)(color_borders.y * 65535.0f), (unsigned short)(color_borders.z * 65535.0f), 65535 };
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                           *map_cache,
                                           color,
                                           proj_func);
        }
        else
            logger->info("Applying cached map overlay...");

        for (size_t i = 0; i < width * height; i++)
        {
            if (map_cache->channel(3)[i] != 0)
                for (int j = 0; j < img.channels(); j++)
                    img.channel(j)[i] = map_cache->channel(j)[i];
        }

        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw map shorelines
    if (draw_shores_overlay)
    {
        if (shores_cache == nullptr || shores_cache->width() != width || shores_cache->height() != height ||
            last_color_shores.w != color_shores.w || last_color_shores.x != color_shores.x ||
            last_color_shores.y != color_shores.y || last_color_shores.z != color_shores.z)
        {
            logger->info("Drawing shores overlay...");
            delete shores_cache;
            shores_cache = new image::Image<uint16_t>(width, height, 4);
            unsigned short color[4] = { (unsigned short)(color_shores.x * 65535.0f), (unsigned short)(color_shores.y * 65535.0f), (unsigned short)(color_shores.z * 65535.0f), 65535 };
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_coastline.shp")},
                                           *shores_cache,
                                           color,
                                           proj_func);
        }
        else
            logger->info("Applying cached shores overlay...");

        for (size_t i = 0; i < width * height; i++)
        {
            if (shores_cache->channel(3)[i] != 0)
                for (int j = 0; j < img.channels(); j++)
                    img.channel(j)[i] = shores_cache->channel(j)[i];
        }

        last_color_borders = color_borders;
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw cities points
    if (draw_cities_overlay)
    {
        if (cities_cache == nullptr || cities_cache->width() != width || cities_cache->height() != height ||
            last_color_cities.w != color_cities.w || last_color_cities.x != color_cities.x ||
            last_color_cities.y != color_cities.y || last_color_cities.z != color_cities.z)
        {
            logger->info("Drawing cities overlay...");
            delete cities_cache;
            cities_cache = new image::Image<uint16_t>(width, height, 4);
            unsigned short color[4] = { (unsigned short)(color_cities.x * 65535.0f), (unsigned short)(color_cities.y * 65535.0f), (unsigned short)(color_cities.z * 65535.0f), 65535 };
            map::drawProjectedCitiesGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                            *cities_cache,
                                            color,
                                            proj_func,
                                            cities_size,
                                            cities_type,
                                            cities_scale_rank);
        }
        else
            logger->info("Applying cached cities overlay...");

        for (size_t i = 0; i < width * height; i++)
        {
            if (cities_cache->channel(3)[i] != 0)
                for (int j = 0; j < img.channels(); j++)
                    img.channel(j)[i] = cities_cache->channel(j)[i];
        }

        last_color_cities = color_cities;
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw qth overlay
    if (draw_qth_overlay)
    {
        if (qth_cache == nullptr || qth_cache->width() != width || qth_cache->height() != height ||
            last_color_qth.w != color_qth.w || last_color_qth.x != color_qth.x ||
            last_color_qth.y != color_qth.y || last_color_qth.z != color_qth.z)
        {
            logger->info("Drawing QTH overlay...");
            delete qth_cache;
            qth_cache = new image::Image<uint16_t>(width, height, 4);
            unsigned short color[4] = { (unsigned short)(color_qth.x * 65535.0f), (unsigned short)(color_qth.y * 65535.0f), (unsigned short)(color_qth.z * 65535.0f), 65535 };
            double qth_lon = satdump::config::main_cfg["satdump_general"]["qth_lon"]["value"].get<double>();
            double qth_lat = satdump::config::main_cfg["satdump_general"]["qth_lat"]["value"].get<double>();
            std::pair<float, float> cc = proj_func(qth_lat, qth_lon,
                                                   img.height(), img.width());

            if (!(cc.first == -1 || cc.second == -1))
            {
                qth_cache->draw_line(cc.first - cities_size * 0.3, cc.second - cities_size * 0.3, cc.first + cities_size * 0.3, cc.second + cities_size * 0.3, color);
                qth_cache->draw_line(cc.first + cities_size * 0.3, cc.second - cities_size * 0.3, cc.first - cities_size * 0.3, cc.second + cities_size * 0.3, color);
                qth_cache->draw_circle(cc.first, cc.second, 0.15 * cities_size, color, true);
                qth_cache->draw_text(cc.first, cc.second + cities_size * 0.15, color, cities_size, "QTH");
            }
        }
        else
            logger->info("Applying cached QTH overlay...");

        for (int i = 0; i < width * height; i++)
        {
            if (qth_cache->channel(3)[i] != 0)
                for (size_t j = 0; j < img.channels(); j++)
                    img.channel(j)[i] = qth_cache->channel(j)[i];
        }

        last_color_qth = color_qth;
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw latlon grid
    if (draw_latlon_overlay)
    {
        if (latlon_cache == nullptr || latlon_cache->width() != width || latlon_cache->height() != height ||
            last_color_latlon.w != color_latlon.w || last_color_latlon.x != color_latlon.x ||
            last_color_latlon.y != color_latlon.y || last_color_latlon.z != color_latlon.z)
        {
            logger->info("Drawing lat/lon overlay...");
            delete latlon_cache;
            latlon_cache = new image::Image<uint16_t>(width, height, 4);
            unsigned short color[4] = { (unsigned short)(color_latlon.x * 65535.0f), (unsigned short)(color_latlon.y * 65535.0f), (unsigned short)(color_latlon.z * 65535.0f), 65535 };
            map::drawProjectedMapLatLonGrid(*latlon_cache, color, proj_func);
        }
        else
            logger->info("Applying lat/lon overlay...");

        for (size_t i = 0; i < width * height; i++)
        {
            if (latlon_cache->channel(3)[i] != 0)
                for (size_t j = 0; j < img.channels(); j++)
                    img.channel(j)[i] = latlon_cache->channel(j)[i];
        }

        last_color_latlon = color_latlon;
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }
}

nlohmann::json OverlayHandler::get_config()
{
    nlohmann::json out;
    out["cities_type"] = cities_type;
    out["cities_size"] = cities_size;
    out["cities_scale_rank"] = cities_scale_rank;
    out["borders_color"] = {color_borders.x, color_borders.y, color_borders.z};
    out["shores_color"] = {color_shores.x, color_shores.y, color_shores.z};
    out["cities_color"] = {color_cities.x, color_cities.y, color_cities.z};
    out["qth_color"] = {color_qth.x, color_qth.y, color_qth.z};
    out["latlon_color"] = {color_latlon.x, color_latlon.y, color_latlon.z};

    out["draw_map_overlay"] = draw_map_overlay;
    out["draw_shores_overlay"] = draw_shores_overlay;
    out["draw_cities_overlay"] = draw_cities_overlay;
    out["draw_qth_overlay"] = draw_qth_overlay;
    out["cities_scale"] = cities_size;
    out["draw_latlon_overlay"] = draw_latlon_overlay;

    return out;
}

void OverlayHandler::set_config(nlohmann::json in, bool status)
{
    if (in.contains("borders_color"))
    {
        std::vector<float> color = in["borders_color"].get<std::vector<float>>();
        color_borders.x = color[0];
        color_borders.y = color[1];
        color_borders.z = color[2];
    }

    if (in.contains("shores_color"))
    {
        std::vector<float> color = in["shores_color"].get<std::vector<float>>();
        color_shores.x = color[0];
        color_shores.y = color[1];
        color_shores.z = color[2];
    }

    if (in.contains("cities_color"))
    {
        std::vector<float> color = in["cities_color"].get<std::vector<float>>();
        color_cities.x = color[0];
        color_cities.y = color[1];
        color_cities.z = color[2];
    }

    if (in.contains("qth_color"))
    {
        std::vector<float> color = in["qth_color"].get<std::vector<float>>();
        color_qth.x = color[0];
        color_qth.y = color[1];
        color_qth.z = color[2];
    }

    if (in.contains("latlon_color"))
    {
        std::vector<float> color = in["latlon_color"].get<std::vector<float>>();
        color_latlon.x = color[0];
        color_latlon.y = color[1];
        color_latlon.z = color[2];
    }

    if (in.contains("cities_size"))
        cities_size = in["cities_size"].get<int>();
    if (in.contains("cities_type"))
        cities_type = in["cities_type"].get<int>();
    if (in.contains("cities_scale_rank"))
        cities_scale_rank = in["cities_scale_rank"].get<int>();

    if (status)
    {
        setValueIfExists(in["draw_map_overlay"], draw_map_overlay);
        setValueIfExists(in["draw_shores_overlay"], draw_shores_overlay);
        setValueIfExists(in["draw_cities_overlay"], draw_cities_overlay);
        setValueIfExists(in["draw_latlon_overlay"], draw_latlon_overlay);
    }
    setValueIfExists(in["cities_scale"], cities_size);
}

void OverlayHandler::set_defaults()
{
    std::vector<float> dcolor_borders = satdump::config::main_cfg["satdump_general"]["default_borders_color"]["value"].get<std::vector<float>>();
    color_borders.x = dcolor_borders[0];
    color_borders.y = dcolor_borders[1];
    color_borders.z = dcolor_borders[2];
    std::vector<float> dcolor_shores = satdump::config::main_cfg["satdump_general"]["default_shores_color"]["value"].get<std::vector<float>>();
    color_shores.x = dcolor_shores[0];
    color_shores.y = dcolor_shores[1];
    color_shores.z = dcolor_shores[2];
    std::vector<float> dcolor_cities = satdump::config::main_cfg["satdump_general"]["default_cities_color"]["value"].get<std::vector<float>>();
    color_cities.x = dcolor_cities[0];
    color_cities.y = dcolor_cities[1];
    color_cities.z = dcolor_cities[2];
    std::vector<float> dcolor_qth = satdump::config::main_cfg["satdump_general"]["default_qth_color"]["value"].get<std::vector<float>>();
    color_qth.x = dcolor_qth[0];
    color_qth.y = dcolor_qth[1];
    color_qth.z = dcolor_qth[2];
    std::vector<float> dcolor_latlon = satdump::config::main_cfg["satdump_general"]["default_latlon_color"]["value"].get<std::vector<float>>();
    color_latlon.x = dcolor_latlon[0];
    color_latlon.y = dcolor_latlon[1];
    color_latlon.z = dcolor_latlon[2];
}

void OverlayHandler::clear_cache()
{
    delete map_cache;
    delete shores_cache;
    delete cities_cache;
    delete qth_cache;
    delete latlon_cache;
    map_cache = shores_cache = cities_cache = qth_cache = latlon_cache = nullptr;
}