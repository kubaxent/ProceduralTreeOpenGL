#version 330


//Zmienne interpolowane

in vec4 l; //interpolowany wektor "do œwiat³a" w przestrzeni oka
in vec4 n; //interpolowany wektor normalny w przestrzeni oka
in vec4 v; //interpolowany wektor "do obserwatora" w przestrzeni oka
in vec2 iTexCoord0; //interpolowane wspó³rzêdne teksturowania dla tekstury w textureMap0
in vec2 iTexCoord1; //interpolowane wspó³rzêdne teksturowania dla tekstury w textureMap1

uniform sampler2D textureMap0; //Jednostka teksturuj¹ca 0
uniform sampler2D textureMap1; //Jednostka teksturuj¹ca 1

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela

void main(void) {
    vec4 kd=mix(texture(textureMap0,iTexCoord0),texture(textureMap1,iTexCoord1),0.25);  //Kolor materia?u dla ?wiat?a  rozpraszanego
    vec4 ld=vec4(1,1,1,1); //Kolor ?wiat?a  rozpraszanego
    vec4 ks=vec4(1,1,1,1); //Kolor odbi? materia?u
    vec4 ls=vec4(1,1,1,1); //Kolor ?wiat?a odbijanego

    vec4 ml=normalize(l);
    vec4 mn=normalize(n);
    vec4 mv=normalize(v);
    vec4 mr=reflect(-ml,mn); //Wektor kierunku odbicia w przestrzeni oka

    float nl=clamp(dot(mn,ml),0,1); //cos k?ta pomi?dzy wektorami n i l
    float rv=pow(clamp(dot(mr,mv),0,1),5555); //cos k?ta pomi?dzy wektorami r i v podniesiony do pot?gi (wyk?adnik Phonga)

	pixelColor=(vec4(kd.rgb*ld.rgb*nl+ks.rgb*ls.rgb*rv,kd.a)*2.0f)+vec4(0.15,0.15,0.15,0);
}
