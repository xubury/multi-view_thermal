#ifndef _AXIS_HPP
#define _AXIS_HPP

#include "RenderTarget.hpp"

class Axis : public RenderTarget {
public:
    explicit Axis(const glm::mat4 &model);

protected:
    void DrawArray() override;

private:
    unsigned int m_VAO;
};


#endif //_AXIS_HPP
