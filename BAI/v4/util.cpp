namespace MMAI::BAI::V4 {
    namespace Util {
        int Damp(int v, int max) {
            return max * std::tanh(static_cast<float>(v) / max);
        }
    }
}
