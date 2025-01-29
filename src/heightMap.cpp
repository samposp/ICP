#include "App.h"

void App::init_hm(void)
{
    // height map
    {
        unsigned int STEP_SIZE = 10;

        std::filesystem::path hm_file("resources/textures/heights.png");
        cv::Mat hmap = cv::imread(hm_file.string(), cv::IMREAD_GRAYSCALE);
        cv::Mat flipedHmap;
        cv::flip(hmap, flipedHmap, 0);

        if (hmap.empty())
            throw std::runtime_error("ERR: Height map empty? File: " + hm_file.string());

        terrain = flipedHmap;

        Mesh height_map = GenHeightMap(flipedHmap, STEP_SIZE); //image, step size
        scene.insert({"height_map", height_map });
        //std::cout << "Note: height map vertices: " << height_map.vertices.size() << std::endl;
    }
}

glm::vec3 App::getPositionOnTerrain(glm::vec3 position) {
    unsigned int SCALE = 2;
    unsigned int STEP_SIZE = 10;

    float position_x = position.x;

    if (position_x < 0)
        position_x = 0;
    else if (position_x > terrain.cols- 2 * STEP_SIZE)
        position_x = terrain.cols- 2 * STEP_SIZE;

    float position_z = position.z;
    if (position_z < 0)
        position_z = 0;
    else if (position_z > terrain.rows- 2 * STEP_SIZE)
        position_z = terrain.rows- 2 * STEP_SIZE;

    int coord_x = floor(position_x / STEP_SIZE) * STEP_SIZE;
    int coord_z = floor(position_z / STEP_SIZE) * STEP_SIZE;

    float h00 = terrain.at<uchar>(cv::Point(coord_x, coord_z)) / SCALE;
    float h01 = terrain.at<uchar>(cv::Point(coord_x, coord_z+STEP_SIZE)) / SCALE;
    float h10 = terrain.at<uchar>(cv::Point(coord_x+STEP_SIZE, coord_z)) / SCALE;
    float h11 = terrain.at<uchar>(cv::Point(coord_x+ STEP_SIZE, coord_z+STEP_SIZE)) / SCALE;

    float factorX = (position_x - coord_x)/STEP_SIZE;
    float factorZ = (position_z - coord_z)/ STEP_SIZE;

    float h_bottom = (h01 - h00) * factorZ + h00;
    float h_top = (h11 - h10) * factorZ + h10;

    float h_final = (h_top - h_bottom) * factorX + h_bottom;

    return glm::vec3(position_x, h_final, position_z);

}

//return bottom left ST coordinate of subtexture
glm::vec2 get_subtex_st(const int x, const int y)
{
    return glm::vec2(x * 1.0f / 16, y * 1.0f / 16);
}

// choose subtexture based on height
glm::vec2 get_subtex_by_height(float height)
{
    if (height > 0.9)
        return get_subtex_st(0, 4); //snow
    else if (height > 0.8)
        return get_subtex_st(3, 4); //ice
    else if (height > 0.5)
        return get_subtex_st(5, 3); //rock
    else if (height > 0.3)
        return get_subtex_st(7, 0); //soil
    else
        return get_subtex_st(0, 0); //grass
}

Mesh App::GenHeightMap(const cv::Mat& hmap, const unsigned int mesh_step_size)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;


    std::cout << "Note: heightmap size:" << hmap.size << ", channels: " << hmap.channels() << std::endl;

    if (hmap.channels() != 1) {
        std::cerr << "WARN: requested 1 channel, got: " << hmap.channels() << std::endl;
    }

    // Create heightmap mesh from TRIANGLES in XZ plane, Y is UP (right hand rule)
    //
    //   3-----2
    //   |    /|
    //   |  /  |
    //   |/    |
    //   0-----1
    //
    //   012,023
    //

    float heightScale = 2;

    for (unsigned int x_coord = 0; x_coord < (hmap.cols - mesh_step_size); x_coord += mesh_step_size)
    {
        for (unsigned int z_coord = 0; z_coord < (hmap.rows - mesh_step_size); z_coord += mesh_step_size)
        {
            // Get The (X, Y, Z) Value For The Bottom Left Vertex = 0
            glm::vec3 p0(x_coord, hmap.at<uchar>(cv::Point(x_coord, z_coord))/ heightScale, z_coord);
            // Get The (X, Y, Z) Value For The Bottom Right Vertex = 1
            glm::vec3 p1(x_coord + mesh_step_size, hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord))/ heightScale, z_coord);
            // Get The (X, Y, Z) Value For The Top Right Vertex = 2
            glm::vec3 p2(x_coord + mesh_step_size, hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size))/ heightScale, z_coord + mesh_step_size);
            // Get The (X, Y, Z) Value For The Top Left Vertex = 3
            glm::vec3 p3(x_coord, hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size))/ heightScale, z_coord + mesh_step_size);

            // Get max normalized height for tile, set texture accordingly
            // Grayscale image returns 0..256, normalize to 0.0f..1.0f by dividing by 256
            float max_h = std::max(hmap.at<uchar>(cv::Point(x_coord, z_coord)) / 256.0f,
                std::max(hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)) / 256.0f,
                    std::max(hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)) / 256.0f,
                        hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)) / 256.0f
                    )));

            // Get texture coords in vertices, bottom left of geometry == bottom left of texture
            glm::vec2 tc0 = get_subtex_by_height(max_h);
            glm::vec2 tc1 = tc0 + glm::vec2(1.0f / 16, 0.0f);       //add offset for bottom right corner
            glm::vec2 tc2 = tc0 + glm::vec2(1.0f / 16, 1.0f / 16);  //add offset for top right corner
            glm::vec2 tc3 = tc0 + glm::vec2(0.0f, 1.0f / 16);       //add offset for bottom leftcorner

            // normals for both triangles, CCW
            glm::vec3 n1 = glm::normalize(glm::cross(p2 - p0, p1 - p0)); // for p1
            glm::vec3 n2 = glm::normalize(glm::cross(p3 - p0, p2 - p0)); // for p3
            //glm::vec3 n1 = glm::normalize(glm::cross(p0 - p1, p0 - p2));// for p1
            //glm::vec3 n2 = glm::normalize(glm::cross(p0 - p2, p0 - p3)); // for p3
            glm::vec3 navg = glm::normalize(n1 + n2);                 // average for p0, p2 - common

            // place indices
            GLuint index0 = vertices.size();
            indices.insert(indices.end(), { index0, index0 + 2, index0 + 1, index0, index0 + 3, index0 + 2 });

            //place vertices and ST to mesh
            Vertex v = Vertex();
            v.Position = p0;
            v.Normal = navg;
            v.TexCoords = tc0;
            vertices.push_back(v);
            v.Position = p1;
            v.Normal = n1;
            v.TexCoords = tc1;
            vertices.push_back(v);
            v.Position = p2;
            v.Normal = navg;
            v.TexCoords = tc2;
            vertices.push_back(v);
            v.Position = p3;
            v.Normal = n2;
            v.TexCoords = tc3;
            vertices.push_back(v);
        }
    }


    Mesh m = Mesh(GL_TRIANGLES, shaders[0], vertices, indices, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    m.texture_id = textureInit("resources/textures/tex_256.png");
    return m;
}