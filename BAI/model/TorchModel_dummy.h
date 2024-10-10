#include "schema/base.h"

namespace MMAI::BAI {
    class TorchModel : public MMAI::Schema::IModel {
    public:
        TorchModel(std::string path, float temperature, uint64_t seed) {
            throw std::runtime_error(
                "MMAI was compiled without executorch or libtorch support and cannot load \"MMAI_MODEL\" files."
            );
        }

        Schema::ModelType getType() { return MMAI::Schema::ModelType::TORCH; };
        std::string getName() { return ""; };
        int getVersion() { return 0; }
        Schema::Side getSide() { return Schema::Side(0); }
        int getAction(const MMAI::Schema::IState * s) { return 0; }
        double getValue(const MMAI::Schema::IState * s) { return 0; }
    };
}
