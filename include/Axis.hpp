#ifndef _AXIS_HPP
#define _AXIS_HPP

#include "RenderTarget.hpp"

class Axis : public RenderTarget {
public:
    using Ptr = std::shared_ptr<Axis>;
public:
    explicit Axis(const glm::mat4 &model);

private:
    void DrawArray() override;

    unsigned int m_VAO;
};


#endif //_AXIS_HPP
