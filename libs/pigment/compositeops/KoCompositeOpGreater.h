/*
 *  SPDX-FileCopyrightText: 2014 Nicholas Guttenberg <ngutten@gmail.com>
 * 
 *  Based on KoCompositeOpBehind.h,
 *  SPDX-FileCopyrightText: 2012 José Luis Vergara <pentalis@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef _KOCOMPOSITEOPGREATER_H_
#define _KOCOMPOSITEOPGREATER_H_

#include "KoCompositeOpBase.h"

/**
 * Greater-than compositor - uses the greater of two alpha values to determine the color
 */
template<class CS_Traits, typename BlendingPolicy>
class KoCompositeOpGreater : public KoCompositeOpBase<CS_Traits, KoCompositeOpGreater<CS_Traits, BlendingPolicy>>
{
    typedef KoCompositeOpBase<CS_Traits, KoCompositeOpGreater<CS_Traits, BlendingPolicy>> base_class;
    typedef typename CS_Traits::channels_type channels_type;
    typedef typename KoColorSpaceMathsTraits<typename CS_Traits::channels_type>::compositetype composite_type;
   
    static const qint8 channels_nb = CS_Traits::channels_nb;
    static const qint8 alpha_pos   = CS_Traits::alpha_pos;

public:
    KoCompositeOpGreater(const KoColorSpace * cs)
        : base_class(cs, COMPOSITE_GREATER, KoCompositeOp::categoryMix()) { }

public:
    template<bool alphaLocked, bool allChannelFlags>
    inline static channels_type composeColorChannels(const channels_type* src, channels_type srcAlpha,
                                                     channels_type*       dst, channels_type dstAlpha,
                                                     channels_type  maskAlpha, channels_type  opacity,
                                                     const QBitArray& channelFlags                    )  {
        using namespace Arithmetic;
                
        if (isUnitValueFuzzy(dstAlpha)) return dstAlpha;
        channels_type appliedAlpha       = mul(maskAlpha, srcAlpha, opacity);
        
        if (isZeroValueFuzzy(appliedAlpha)) return dstAlpha;
        channels_type newDstAlpha;
        
        float dA = scale<float>(dstAlpha);
        
        float w = 1.0/(1.0+exp(-40.0*(dA - scale<float>(appliedAlpha))));
		float a = dA*w + scale<float>(appliedAlpha)*(1.0-w);               
        if (a < 0.0f) {
            a = 0.0f; 
        }
        if (a > 1.0f) {
            a = 1.0f;
        }
        
        // For a standard Over, the resulting alpha is: a = opacity*dstAlpha + (1-opacity)*srcAlpha
        // Let us assume we're blending with a color with srcAlpha = 1 here
        // Therefore, opacity = (1.0 - a)/(1.0 - dstAlpha)
        if (a<dA) a = dA;
		float fakeOpacity = 1.0f - (1.0f - a)/(1.0f - dA + 1e-16f);
        newDstAlpha=scale<channels_type>(a);

        if (isZeroValueFuzzy(newDstAlpha)) {
          // just do nothing with color channels and return null opacity
        } else if (!isZeroValueFuzzy(dstAlpha)) {
            for (qint8 channel = 0; channel < channels_nb; ++channel)
                if(channel != alpha_pos && (allChannelFlags || channelFlags.testBit(channel)))
                {
                    typedef typename KoColorSpaceMathsTraits<channels_type>::compositetype composite_type;

                    channels_type dstMult = mul(BlendingPolicy::toAdditiveSpace(dst[channel]), dstAlpha);
                    channels_type srcMult = mul(BlendingPolicy::toAdditiveSpace(src[channel]), unitValue<channels_type>());
                    channels_type blendedValue = lerp(dstMult, srcMult, scale<channels_type>(fakeOpacity));

                    composite_type normedValue = KoColorSpaceMaths<channels_type>::divide(blendedValue, newDstAlpha);

                    dst[channel] = BlendingPolicy::fromAdditiveSpace(KoColorSpaceMaths<channels_type>::clampAfterScale(normedValue));
				}
        }       
        else {
            // don't blend if the color of the destination is undefined (has zero opacity)
            // copy the source channel instead
            for (qint8 channel = 0; channel < channels_nb; ++channel)
                if(channel != alpha_pos && (allChannelFlags || channelFlags.testBit(channel)))
                    dst[channel] = src[channel];
        }

        return newDstAlpha;
    }
};

#endif  // _KOCOMPOSITEOPGREATER_H_
