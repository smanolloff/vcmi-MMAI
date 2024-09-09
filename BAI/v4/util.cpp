namespace MMAI::BAI::V4 {
    namespace Util {
        int Damp(int v, int max) {
            return std::round(max * std::tanh(static_cast<double>(v) / max));
        }
    }
}
