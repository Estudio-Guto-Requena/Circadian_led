#import pygame
import pandas as pd
import matplotlib.pyplot as plt
from scipy import interpolate
import numpy as np
import time
import colorsys

rule = pd.read_csv("./values.csv")
x = rule.hora
r = rule.R
g = rule.G
b = rule.B
a = rule.A
interR = interpolate.interp1d(x, r, kind = 'linear')
interG = interpolate.interp1d(x, g, kind = 'linear')
interB = interpolate.interp1d(x, b, kind = 'linear')
interA = interpolate.interp1d(x, b, kind = 'linear')
#print(rule)
#rule.plot(style=['x','r','g','b',''])
#plt.show()

#pygame.init()

#surface = pygame.display.set_mode((1440,1440))
print("int colors[1440][3] = {")
for i in range(1, 1441):
    color = colorsys.rgb_to_hsv(interR(i).tolist(), interG(i).tolist(), interB(i).tolist())
    color = [color[0], color[1], color[2]]
    #pygame.draw.rect(surface, color, pygame.Rect(i,0,1,1440))
    #pygame.display.flip()
    if(i < 200):
        color[2] = 0
    elif(200<i<=300):
        color[2] = (12*i/25)-80
    elif(300<i<=315):
        color[2] = (64*i/15)-1216
    elif(315<i<=360):
        color[2] = (97*i/45)-556
    elif(360<i<=1120):
        color[2] = 220
    elif(1120<i<=1150):
        color[2] = (11860/3)-(10*i/3)
    elif(1150<i<=1180):
        color[2] = (7744/3)-(32*i/15)
    elif(1180<i<=1220):
        color[2] = 1480-(6*i/5)
    else:
        color[2] = 0

    print("{"+str(int(color[0]*255))+","+str(int(color[1]*255))+","+str(int(color[2]))+"},")



#while(1):
#    continue
