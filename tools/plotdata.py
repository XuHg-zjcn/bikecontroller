#!/usr/bin/env python3
#######################################################################
# Copyright (C) 2024  徐瑞骏(科技骏马)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
########################################################################
import sys
import numpy as np
import matplotlib.pyplot as plt

METER_PER_CNT = 1.52

if __name__ == "__main__":
    fn = sys.argv[1]
    f = open(fn, 'rb')
    data = f.read()
    data_np = np.frombuffer(data, np.uint32)
    x_m = np.arange(len(data_np))*METER_PER_CNT
    t_s = np.cumsum(data_np.astype(np.uint64))/100e3
    v_kmh = (METER_PER_CNT*3.6)/(data_np/100e3)
    plt.subplot(211)
    plt.plot(x_m, v_kmh)
    plt.subplot(212)
    plt.plot(t_s, v_kmh)
    plt.show()
