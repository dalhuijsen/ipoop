
assertInterpolation({
  property: 'text-decoration-color',
  from: 'orange',
  to: 'blue'
}, [
  {at: -5, is: '#ffff00'},
  {at: -0.4, is: '#ffe700'},
  {at: 0, is: 'orange'}, // ffa500
  {at: 0.2, is: '#cc8433'},
  {at: 0.6, is: '#664299'},
  {at: 1, is: 'blue'}, // 0000ff
  {at: 1.5, is: 'blue'}
]);
