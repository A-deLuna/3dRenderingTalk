document.addEventListener('keydown', leftright);
function leftright(evnt) {
  var aTag = null;
  if (evnt.code == "ArrowLeft") {
    aTag = document.getElementById("left");
  }
  if (evnt.code == "ArrowRight") {
    aTag = document.getElementById("right");
  }
  if (aTag) {
    aTag.click();
  }
}
