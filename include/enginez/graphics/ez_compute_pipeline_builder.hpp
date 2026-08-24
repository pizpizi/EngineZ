#pragma once

#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_types.hpp"
#include <vector>

namespace enginez::graphics {
    class ezPipelineBuilder {
      private:
        ezEngine& engine;
        std::vector<DescriptorSet> descriptorSets;
        std::vector<PushConstantRange> pushConstants;

      public:
        ezPipelineBuilder(ezEngine& engine): engine(engine){}

        
    };

} // namespace enginez::graphics