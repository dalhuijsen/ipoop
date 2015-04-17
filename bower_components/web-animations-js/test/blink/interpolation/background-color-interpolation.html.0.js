
assertInterpolation({
  property: 'background-color',
  from: 'white',
  to: 'orange'
}, [
  {at: -0.3, is: 'white'},
  {at: 0, is: 'white'},
  {at: 0.3, is: 'rgb(255, 228, 179)'},
  {at: 0.6, is: 'rgb(255, 201, 102)'},
  {at: 1, is: 'orange'},
  {at: 1.5, is: 'rgb(255, 120, 0)'},
]);
assertInterpolation({
  property: 'background-color',
  from: 'initial',
  to: 'transparent'
}, [
  {at: -0.3, is: 'transparent'},
  {at: 0, is: 'transparent'},
  {at: 0.3, is: 'transparent'},
  {at: 0.6, is: 'transparent'},
  {at: 1, is: 'transparent'},
  {at: 1.5, is: 'transparent'},
]);
