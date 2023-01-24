export default {
  // ---------------------------------------------
  // userAgent
  // ---------------------------------------------
  checkUA () {
    var device = 'PC'
    var ua = navigator.userAgent
    if ((ua.indexOf('Android') > 0 && ua.indexOf('Mobile') === -1) || ua.indexOf('iPad') > 0) {
      device = 'TB'
    } else if ((ua.indexOf('iPhone') > 0 && ua.indexOf('iPad') === -1) || ua.indexOf('iPod') > 0 || (ua.indexOf('Android') > 0 && ua.indexOf('Mobile') > 0)) {
      device = 'SP'
    }

    return device
  },
  // ---------------------------------------------
  // shuffle
  // ---------------------------------------------
  shuffleAry (ary) {
    for (var i = ary.length; i > 1; i--) {
      var k = Math.floor(Math.random() * i);
      [ary[k], ary[i - 1]] = [ary[i - 1], ary[k]]
    }
    return ary
  },
  // ---------------------------------------------
  // 画像読み込み
  // ---------------------------------------------
  loadImages (ref, func) {
    let imageLoadCt = 0
    let imageLoadCtLength = 0
    const targets = ref.querySelectorAll('img')
    for (let i = 0; i < targets.length; i++) {
      loadedImage(targets[i].getAttribute('src'))
    }

    function loadedImage (img) {
      const image = new Image()
      imageLoadCtLength++
      image.onload = () => {
        imageLoadCt++
        console.log('load complete' + Math.floor(imageLoadCt / imageLoadCtLength * 100) + '%')
        if (imageLoadCt >= imageLoadCtLength) {
          func()
        }
      }
      image.src = img
    }
  }

}
