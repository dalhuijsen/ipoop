

[-8, -5, -2, 1, 5, 10, 12].forEach(function(zIndex, i) {
  var layerReference = document.createElement('div');
  layerReference.classList.add('layer-reference');
  layerReference.style.zIndex = zIndex;
  layerReference.style.top = '0px';
  layerReference.style.left = 50 + (i * 50) + 'px';
  layerReference.textContent = 'Z ' + zIndex;
  document.body.appendChild(layerReference);
});
assertInterpolation({
  property: 'z-index',
  from: '-5',
  to: '5'
}, [
  {at: -0.3, is: '-8'},
  {at: 0, is: '-5'},
  {at: 0.3, is: '-2'},
  {at: 0.6, is: '1'},
  {at: 1, is: '5'},
  {at: 1.5, is: '10'},
]);
afterTest(function() {
  var actives = document.querySelectorAll('.active');
  var replicas = document.querySelectorAll('.replica');
  for (var i = 0; i < actives.length; i++) {
    actives[i].style.top = 50 + (i * 40) + 'px';
    replicas[i].style.top = 60 + (i * 40) + 'px';
  }
});
