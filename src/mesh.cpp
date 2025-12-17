// Created by dengq on 8/29/25.
//
#include "mesh.h"
#include "stb_image.h"
#include <iostream>
#include "tiny_obj_loader.h"
#include <unordered_set>

// Vertex layout: [px,py,pz, nx,ny,nz, u,v]

bool Mesh::loadOBJ_(const std::string &path) {
    vertices.clear();
    indices.clear();

    tinyobj::ObjReaderConfig config;
    config.triangulate = false; // we'll triangulate ourselves (fan) to match your logic
    config.vertex_color = false;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config)) {
        if (!reader.Error().empty())
            std::cerr << "tinyobj error: " << reader.Error() << "\n";
        return false;
    }
    if (!reader.Warning().empty()) {
        std::cerr << "tinyobj warning: " << reader.Warning() << "\n";
    }

    const tinyobj::attrib_t &attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();

    auto getPos = [&](int vi) -> glm::vec3 {
        const float *v = attrib.vertices.data() + 3 * vi;
        return glm::vec3(v[0], v[1], v[2]);
    };

    auto getUV = [&](int ti) -> glm::vec2 {
        const float *t = attrib.texcoords.data() + 2 * ti;
        return glm::vec2(t[0], t[1]);
    };

    uint32_t baseIndex = 0;

    for (const auto &shape: shapes) {
        // --- 1) Shape centroid over unique referenced vertices
        glm::dvec3 centroid(0.0, 0.0, 0.0);
        size_t count = 0;
        std::unordered_set<int> used;
        used.reserve(shape.mesh.indices.size());
        for (const auto &idx: shape.mesh.indices) {
            if (idx.vertex_index < 0) continue;
            if (used.insert(idx.vertex_index).second) {
                glm::vec3 p = getPos(idx.vertex_index);
                centroid += glm::dvec3(p);
                ++count;
            }
        }
        if (count > 0) centroid /= static_cast<double>(count);

        // --- 2) Iterate faces and triangulate (fan) if needed
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv < 3) {
                index_offset += fv;
                continue;
            }

            // Collect the whole polygon's positions (for normal + fan)
            std::vector<glm::vec3> facePos(fv);
            for (int v = 0; v < fv; ++v) {
                const auto &idx = shape.mesh.indices[index_offset + v];
                if (idx.vertex_index < 0) {
                    /* malformed face */
                    continue;
                }
                facePos[v] = getPos(idx.vertex_index);
            }

            // Fan triangulation: (0, k, k+1)
            for (int k = 1; k < fv - 1; ++k) {
                const glm::vec3 &p0 = facePos[0];
                const glm::vec3 &p1 = facePos[k];
                const glm::vec3 &p2 = facePos[k + 1];

                // --- 3) Flat normal
                glm::vec3 e1 = p1 - p0;
                glm::vec3 e2 = p2 - p0;
                glm::vec3 n = glm::cross(e1, e2);
                float len = glm::length(n);
                if (len > 0.0f) n /= len;
                else n = glm::vec3(0, 0, 1);

                // --- 4) Flip if pointing inward (toward centroid)
                glm::vec3 faceCenter = (p0 + p1 + p2) / 3.0f;
                glm::vec3 outDir = faceCenter - glm::vec3(centroid);
                if (glm::dot(n, outDir) < 0.0f) n = -n;


                // Helper to emit a single vertex (pos, n, uv) + push index
                auto emit = [&](int localFaceVertex) {
                    const auto &idx = shape.mesh.indices[index_offset + localFaceVertex];
                    const glm::vec3 p = getPos(idx.vertex_index);

                    vertices.push_back(p.x);
                    vertices.push_back(p.y);
                    vertices.push_back(p.z);

                    vertices.push_back(n.x);
                    vertices.push_back(n.y);
                    vertices.push_back(n.z);

                    float u = 0.0f, v = 0.0f;
                    if (idx.texcoord_index >= 0 &&
                        static_cast<size_t>(2 * idx.texcoord_index + 1) < attrib.texcoords.size()) {
                        glm::vec2 uv = getUV(idx.texcoord_index);
                        u = uv.x;
                        v = uv.y;
                    }
                    vertices.push_back(u);
                    vertices.push_back(v);

                    indices.push_back(baseIndex++);
                };

                // Triangle: (0, k, k+1)
                emit(0);
                emit(k);
                emit(k + 1);
            }

            index_offset += fv;
        }
    }

    return !vertices.empty() && !indices.empty();
}

void Mesh::createBuffers_() {
    const GLint posLoc = glGetAttribLocation(shaderProgramID, "aPos");
    const GLint norLoc = glGetAttribLocation(shaderProgramID, "aNor");
    const GLint uvLoc = glGetAttribLocation(shaderProgramID, "aTexCoord");

    GLsizei stride = 8 * sizeof(float);
    std::vector<VertexAttribute> attributes = {
        VertexAttribute{static_cast<GLuint>(posLoc), 3, GL_FLOAT, GL_FALSE, stride, 0},
        VertexAttribute{static_cast<GLuint>(norLoc), 3, GL_FLOAT, GL_FALSE, stride, 3 * sizeof(float)},
        VertexAttribute{static_cast<GLuint>(uvLoc), 2, GL_FLOAT, GL_FALSE, stride, 6 * sizeof(float)}
    };

    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(),
                 GL_STATIC_DRAW);

    for (const auto &attr: attributes) {
        glVertexAttribPointer(attr.pos,
                              attr.size,
                              attr.type,
                              attr.normalized,
                              attr.stride,
                              reinterpret_cast<const void *>(attr.offset));
        glEnableVertexAttribArray(attr.pos);
    }

    glBindVertexArray(0);
}

Mesh::Mesh(const std::string &path,
           GLuint id) {
    shaderProgramID = id;
    //    M = Transformation{"model", shaderProgramID, glGetUniformLocation(shaderProgramID, "model"), glm::mat4(1.0f)};
    //    V = Transformation{"view", shaderProgramID, glGetUniformLocation(shaderProgramID, "view"), glm::mat4(1.0f)};
    //    P = Transformation{"projection", shaderProgramID, glGetUniformLocation(shaderProgramID, "projection"), glm::mat4(1.0f)};
    loadOBJ_(path);
    //    glUniformMatrix4fv(M.layout, 1, GL_FALSE, glm::value_ptr(M.m));
    //    glUniformMatrix4fv(V.layout, 1, GL_FALSE, glm::value_ptr(V.m));
    //    glUniformMatrix4fv(P.layout, 1, GL_FALSE, glm::value_ptr(P.m));

    createBuffers_();
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::draw() const {
    bind();
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    unbind();
}

void Mesh::bind() const {
    glBindVertexArray(VAO);
}

void Mesh::unbind() const {
    glBindVertexArray(0);
}

void Mesh::cleanup() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}
