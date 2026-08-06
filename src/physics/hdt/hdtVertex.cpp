#ifdef USE_BULLET

#include "hdtVertex.h"

namespace hdt
{
	void Vertex::sortWeight()
	{
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 3; ++j) {
				if (m_weight[j] < m_weight[j + 1]) {
					std::swap(m_weight[j], m_weight[j + 1]);
					std::swap(m_boneIdx[j], m_boneIdx[j + 1]);
				}
			}
		}
	}
}

#endif  // USE_BULLET
